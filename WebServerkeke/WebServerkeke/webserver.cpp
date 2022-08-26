// socket头文件
#include <winsock.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>


#include "webserver.h"
#include "sqlconnectionpool.h"
#include "log.h"

WebServer::WebServer(int port, bool syncLogWrite, int triggerMode, bool optLinger, int sqlNum, int threadNum, bool closeLog, int actorMode) :
	port_(port), sync_log_write_(syncLogWrite), trigger_mode_(triggerMode), opt_linger_(optLinger), sql_num_(sqlNum), thread_num_(threadNum), close_log_(closeLog), actor_mode_(actorMode),
	// 列表初始化用new,epoller_这些必须是个指针
	server_close_(false), epoller_(new Epoller()), timer_(new Timer()) {

	// getcwd定义在unistd.h中,获得当前工作目录的绝对路径给srcDir_
	HttpConn::src_dir_ = getcwd(nullptr, 256);
	// user_count刻画了服务器连上了多少个用户，是属于类的性质，因此是static
	// 静态成员变量定义应放在.cc中，且最好是HttpConn这个类的cc
	HttpConn::user_count_ = 0;
	initEventMode(triggerMode);
	initListenFd();
	// 完成连接池的初始化，饿汉式连接池实例，之后的instance()都只是拿到已经初始化好的类对象
	SqlConnectionPool::Instance()->init();
	Log::Instance()->init(0);

	// webserver初始化需要传入数据库的登录用户和密码以便于webserver能访问到数据库


}

WebServer::~WebServer() {
	close(listen_fd_);
	SqlConnectionPool::Instance()->closePool();

}

void WebServer::initEventMode(int trigger_mode) {
	// 主要设置webserver中的epoll事件类型
	listen_event_ = EPOLLRDHUP;
	conn_event_ = EPOLLRDHUP | EPOLLONESHOT;
	switch (trigger_mode) {
	// 默认是LT，根据trigger_mode修改conn_event_和listen_event_中的若干为ET
	case 0:
		conn_event_ |= EPOLLET;
		break;
	case 1:
		listen_event_ |= EPOLLET;
		break;
	case 2:
		conn_event_ |= EPOLLET;
		listen_event_ |= EPOLLET;
		break;
	default:
		break;
	}
	// 只有conn_event_有EPOLLET时isET为true
	HttpConn::is_ET_ = conn_event_ & EPOLLET;
}

void WebServer::setFdNonBlock(int fd) {
	// linux下才有fcntl这个函数
	// 获取fd对应文件原来的flag值
	int flag = fcntl(fd, F_GETFD, 0);
	// 修改相应的flag值为非阻塞模式
	return fcntl(fd, F_SETFL, flag | 0_NONBLOCK);

}

void WebServer::initListenFd() {
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	// htonl/htons：将端口号由主机字节序转换为网络字节序
	addr.sin_port = htons(port_);
	// linger结构体设置了tcp连接断开的方式
	struct linger opt_linger;
	if (opt_linger_) {
		// 优雅关闭tcp连接，直到剩余数据发送完毕或超时
		opt_linger.l_linger = 1;
		opt_linger.l_onoff = 1;
	}
	// 创建socket
	listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
	// setsockopt,socket初始化linger
	int ret = setsockopt(listen_fd_, SOL_SOCKET, SO_LINGER, (const char*)&opt_linger, sizeof(opt_linger));

	// setsockopt,端口复用，最后一个套接字会正常接收数据
	int optval = 1;
	ret = setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, sizeof(int));
	// 结构体指针，绑定socket地址
	ret = bind(listen_fd_, (struct sockaddr*)&addr, sizeof(addr));
	
	ret = listen(listen_fd_, 6);
	// EPOLLIN表示对应的fd可以读
	ret = epoller_->addFd(listen_fd_, listen_event_ | EPOLLIN);

	// 设置为非阻塞，如果listen_fd_没有连接，则程序不会阻塞在这
	setFdNonBlock(listen_fd_);
}


void WebServer::closeConn(HttpConn* client) {
	epoller_->deleteFd(client->getFd());
	client->close();
}

void WebServer::dealListen() {
	
	struct sockaddr_in addr;
	// socklen_t定义在<sys/socket.h>中
	socklen_t len = sizeof(addr);
	// 如果while里面是true说明是ET模式，则要循环读完所有数据，否则就是LT，只执行do里面一遍
	do {
		// 这里的fd已经是对应conn的fd
		// accpet函数会存储相应客户端的信息(ip port)到addr里，fd指向的就是addr代表的位置
		int fd = accept(listen_fd_, (struct sockaddr*)&addr, &len);
		if (HttpConn::user_count_ > max_fd_) {
			LOG_ERROR("server busy")
		}
		// 添加http client
		// map的[]符号可以添加元素，在这里添加了一个fd和默认http对象的映射，再init
		users_[fd].init(fd, addr);
		// 添加timer，不要在这里封装TimerNode，不然又要引入TimerNode头文件
		// 封装TimerNode交给Timer去做，类之间要解耦，在类中添加方法
		int time_out = 60000;
		// 用bind绑定timer的回调函数,&users_[fd]作为函数参数，该函数将会作为每个timer的一个“属性”，在某个时刻被调用
		// 这里回调是closeConn，可能在timer过期调用，或者手动close调用
		timer_->addTimer(fd, time_out, std::bind(&WebServer::closeConn, this, &users_[fd]));
		// 允许相应conn的fd可以读
		epoller_->addFd(fd, EPOLLIN | conn_event_);
		setFdNonBlock(fd);
		
	// listen是否为ET模式
	} while (listen_event_ & EPOLLET);

}

// 完成数据的解析，以及组装response报文并写到缓冲区里
void WebServer::onProcess(HttpConn* client) {
	// 处理数据也是交给http类
	if (client->process()) {
		// 如果process完成，将对应的fd改为就绪写，等待下一次epoll_wait将数据写到对应buff中
		epoller_->modifyFd(client->getFd, conn_event_ | EPOLLOUT);
	}
	else {
		epoller_->modifyFd(client->getFd, conn_event_ | EPOLLIN);
	}
}

// 当外界传数据到server，先读取数据再处理数据
// reactor模式，主线程负责建立连接，子线程负责处理发送数据
void WebServer::onRead(HttpConn* client) {

	int ret = -1;
	int read_error = 0;
	// 数据读取最后是交给http类，client->read里面有个while，所以是ET模式
	ret = client->read(&read_error);
	// 怎么保证read_buff一次读完的是整个请求数据？以及怎么保证发送数据的时候能一次发送完？
	onProcess(client);
}

// server写数据到客户端
// 当有新连接时，首先读取数据并对数据处理，然后等待相应的fd可写，再执行这个函数来写数据到浏览器端
void WebServer::onWrite(HttpConn* client) {

	int ret = -1;
	int write_error = 0;
	ret = client->write(&write_error);
	// 传输完毕
	if (client->toWriteByte() == 0) {
		// 长连接，重置http实例并注册读事件，不关闭连接，否则直接到closeConn
		if (client->isKeepAlive()) {
			onProcess(client);
			return;
		}
	}
	else if (ret < 0) {
		// 写缓冲区满了，下次继续传输
		if (write_error == EAGAIN) {
			// 让该fd继续监听写事件，隔一段时间就会触发epoll_wait
			// 如果fd监听的是EPOLLIN，那么只有当客户端发送请求时会触发epoll_wait
			// 事件类型为epollout，表示可以继续写
			// 重新将fd注册为写事件，等待下一次触发再写到浏览器
			epoller_->modifyFd(client->getFd(), conn_event_ | EPOLLOUT);
			return;
		}
	}
	closeConn(client);
}

// 修改client对应TimeNode的过期时间
void WebServer::extendTime(HttpConn* client) {
	int time_out = 60000;
	timer_->adjust(client->getFd(), time_out);
}

void WebServer::dealRead(HttpConn* client) {
	// 当某个http fd有数据读时，首先需要将这个fd的超时时间延长
	extendTime(client);
	// bind返回一个function对象，为什么变成了一个右值引用？
	// reactor模式，主线程负责读写，工作线程（线程池中的线程）负责处理逻辑
	thread_pool_->addTask(std::bind(&WebServer::onRead, this, client));
	
}

void WebServer::dealWrite(HttpConn* client) {
	extendTime(client);
	thread_pool_->addTask(std::bind(&WebServer::onWrite, this, client));
}

void WebServer::start() {
	if (server_close_) {
		LOG_ERROR("server close now !");
	}
	/* epoll wait timeout == -1 无事件将阻塞 */
	int time_ms = -1;
	while (!server_close_) {
		time_ms = timer_->getNextTick();
		// reactor模式，主线程中通过epoller和dealListen来实现客户端连接的建立和分发，具体的read和write系统调用由子线程完成(dealread & dealwrite)，可能阻塞在这上面
		// timeMS表示经过这么长时间Wait会返回有多少个事件
		int event_cnt = epoller_->wait(time_ms);
		for (int i = 0; i < event_cnt; i++) {
			int fd = epoller_->getEventFd(i);
			uint32_t events = epoller_->getEvent(i);
			if (fd == listen_fd_) {
				// 有新连接
				dealListen();
			}
			// 对已有http连接处理
			// 工作线程处理完http连接后，往epoll内核事件表上注册该socket的写就绪事件。再由epoll_wait等待socket可写，当socket可写，往socket写入处理客户请求的结果
			// 用户发来请求报文后对应的fd上有可读事件
			else if (events & EPOLLIN) {
				dealRead(&users_[fd]);
			}
			else if (events & EPOLLOUT) {
				dealWrite(&users_[fd]);
			}
			// & (| | |)这种写法表示只有后面三个事件里面有一个跟event匹配就行
			// & 和 | 表示位运算，两个相同的数字位运算则返回非零的数
			else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
				closeConn(&users_[fd]);
			}
			else {
				LOG_ERROR("unexpected event");
			}
		}

	}
}

