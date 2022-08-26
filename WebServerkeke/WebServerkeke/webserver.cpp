// socketͷ�ļ�
#include <winsock.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>


#include "webserver.h"
#include "sqlconnectionpool.h"
#include "log.h"

WebServer::WebServer(int port, bool syncLogWrite, int triggerMode, bool optLinger, int sqlNum, int threadNum, bool closeLog, int actorMode) :
	port_(port), sync_log_write_(syncLogWrite), trigger_mode_(triggerMode), opt_linger_(optLinger), sql_num_(sqlNum), thread_num_(threadNum), close_log_(closeLog), actor_mode_(actorMode),
	// �б��ʼ����new,epoller_��Щ�����Ǹ�ָ��
	server_close_(false), epoller_(new Epoller()), timer_(new Timer()) {

	// getcwd������unistd.h��,��õ�ǰ����Ŀ¼�ľ���·����srcDir_
	HttpConn::src_dir_ = getcwd(nullptr, 256);
	// user_count�̻��˷����������˶��ٸ��û���������������ʣ������static
	// ��̬��Ա��������Ӧ����.cc�У��������HttpConn������cc
	HttpConn::user_count_ = 0;
	initEventMode(triggerMode);
	initListenFd();
	// ������ӳصĳ�ʼ��������ʽ���ӳ�ʵ����֮���instance()��ֻ���õ��Ѿ���ʼ���õ������
	SqlConnectionPool::Instance()->init();
	Log::Instance()->init(0);

	// webserver��ʼ����Ҫ�������ݿ�ĵ�¼�û��������Ա���webserver�ܷ��ʵ����ݿ�


}

WebServer::~WebServer() {
	close(listen_fd_);
	SqlConnectionPool::Instance()->closePool();

}

void WebServer::initEventMode(int trigger_mode) {
	// ��Ҫ����webserver�е�epoll�¼�����
	listen_event_ = EPOLLRDHUP;
	conn_event_ = EPOLLRDHUP | EPOLLONESHOT;
	switch (trigger_mode) {
	// Ĭ����LT������trigger_mode�޸�conn_event_��listen_event_�е�����ΪET
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
	// ֻ��conn_event_��EPOLLETʱisETΪtrue
	HttpConn::is_ET_ = conn_event_ & EPOLLET;
}

void WebServer::setFdNonBlock(int fd) {
	// linux�²���fcntl�������
	// ��ȡfd��Ӧ�ļ�ԭ����flagֵ
	int flag = fcntl(fd, F_GETFD, 0);
	// �޸���Ӧ��flagֵΪ������ģʽ
	return fcntl(fd, F_SETFL, flag | 0_NONBLOCK);

}

void WebServer::initListenFd() {
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	// htonl/htons�����˿ں��������ֽ���ת��Ϊ�����ֽ���
	addr.sin_port = htons(port_);
	// linger�ṹ��������tcp���ӶϿ��ķ�ʽ
	struct linger opt_linger;
	if (opt_linger_) {
		// ���Źر�tcp���ӣ�ֱ��ʣ�����ݷ�����ϻ�ʱ
		opt_linger.l_linger = 1;
		opt_linger.l_onoff = 1;
	}
	// ����socket
	listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
	// setsockopt,socket��ʼ��linger
	int ret = setsockopt(listen_fd_, SOL_SOCKET, SO_LINGER, (const char*)&opt_linger, sizeof(opt_linger));

	// setsockopt,�˿ڸ��ã����һ���׽��ֻ�������������
	int optval = 1;
	ret = setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, sizeof(int));
	// �ṹ��ָ�룬��socket��ַ
	ret = bind(listen_fd_, (struct sockaddr*)&addr, sizeof(addr));
	
	ret = listen(listen_fd_, 6);
	// EPOLLIN��ʾ��Ӧ��fd���Զ�
	ret = epoller_->addFd(listen_fd_, listen_event_ | EPOLLIN);

	// ����Ϊ�����������listen_fd_û�����ӣ�����򲻻���������
	setFdNonBlock(listen_fd_);
}


void WebServer::closeConn(HttpConn* client) {
	epoller_->deleteFd(client->getFd());
	client->close();
}

void WebServer::dealListen() {
	
	struct sockaddr_in addr;
	// socklen_t������<sys/socket.h>��
	socklen_t len = sizeof(addr);
	// ���while������true˵����ETģʽ����Ҫѭ�������������ݣ��������LT��ִֻ��do����һ��
	do {
		// �����fd�Ѿ��Ƕ�Ӧconn��fd
		// accpet������洢��Ӧ�ͻ��˵���Ϣ(ip port)��addr�fdָ��ľ���addr�����λ��
		int fd = accept(listen_fd_, (struct sockaddr*)&addr, &len);
		if (HttpConn::user_count_ > max_fd_) {
			LOG_ERROR("server busy")
		}
		// ���http client
		// map��[]���ſ������Ԫ�أ������������һ��fd��Ĭ��http�����ӳ�䣬��init
		users_[fd].init(fd, addr);
		// ���timer����Ҫ�������װTimerNode����Ȼ��Ҫ����TimerNodeͷ�ļ�
		// ��װTimerNode����Timerȥ������֮��Ҫ�����������ӷ���
		int time_out = 60000;
		// ��bind��timer�Ļص�����,&users_[fd]��Ϊ�����������ú���������Ϊÿ��timer��һ�������ԡ�����ĳ��ʱ�̱�����
		// ����ص���closeConn��������timer���ڵ��ã������ֶ�close����
		timer_->addTimer(fd, time_out, std::bind(&WebServer::closeConn, this, &users_[fd]));
		// ������Ӧconn��fd���Զ�
		epoller_->addFd(fd, EPOLLIN | conn_event_);
		setFdNonBlock(fd);
		
	// listen�Ƿ�ΪETģʽ
	} while (listen_event_ & EPOLLET);

}

// ������ݵĽ������Լ���װresponse���Ĳ�д����������
void WebServer::onProcess(HttpConn* client) {
	// ��������Ҳ�ǽ���http��
	if (client->process()) {
		// ���process��ɣ�����Ӧ��fd��Ϊ����д���ȴ���һ��epoll_wait������д����Ӧbuff��
		epoller_->modifyFd(client->getFd, conn_event_ | EPOLLOUT);
	}
	else {
		epoller_->modifyFd(client->getFd, conn_event_ | EPOLLIN);
	}
}

// ����紫���ݵ�server���ȶ�ȡ�����ٴ�������
// reactorģʽ�����̸߳��������ӣ����̸߳�����������
void WebServer::onRead(HttpConn* client) {

	int ret = -1;
	int read_error = 0;
	// ���ݶ�ȡ����ǽ���http�࣬client->read�����и�while��������ETģʽ
	ret = client->read(&read_error);
	// ��ô��֤read_buffһ�ζ�����������������ݣ��Լ���ô��֤�������ݵ�ʱ����һ�η����ꣿ
	onProcess(client);
}

// serverд���ݵ��ͻ���
// ����������ʱ�����ȶ�ȡ���ݲ������ݴ���Ȼ��ȴ���Ӧ��fd��д����ִ�����������д���ݵ��������
void WebServer::onWrite(HttpConn* client) {

	int ret = -1;
	int write_error = 0;
	ret = client->write(&write_error);
	// �������
	if (client->toWriteByte() == 0) {
		// �����ӣ�����httpʵ����ע����¼������ر����ӣ�����ֱ�ӵ�closeConn
		if (client->isKeepAlive()) {
			onProcess(client);
			return;
		}
	}
	else if (ret < 0) {
		// д���������ˣ��´μ�������
		if (write_error == EAGAIN) {
			// �ø�fd��������д�¼�����һ��ʱ��ͻᴥ��epoll_wait
			// ���fd��������EPOLLIN����ôֻ�е��ͻ��˷�������ʱ�ᴥ��epoll_wait
			// �¼�����Ϊepollout����ʾ���Լ���д
			// ���½�fdע��Ϊд�¼����ȴ���һ�δ�����д�������
			epoller_->modifyFd(client->getFd(), conn_event_ | EPOLLOUT);
			return;
		}
	}
	closeConn(client);
}

// �޸�client��ӦTimeNode�Ĺ���ʱ��
void WebServer::extendTime(HttpConn* client) {
	int time_out = 60000;
	timer_->adjust(client->getFd(), time_out);
}

void WebServer::dealRead(HttpConn* client) {
	// ��ĳ��http fd�����ݶ�ʱ��������Ҫ�����fd�ĳ�ʱʱ���ӳ�
	extendTime(client);
	// bind����һ��function����Ϊʲô�����һ����ֵ���ã�
	// reactorģʽ�����̸߳����д�������̣߳��̳߳��е��̣߳��������߼�
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
	/* epoll wait timeout == -1 ���¼������� */
	int time_ms = -1;
	while (!server_close_) {
		time_ms = timer_->getNextTick();
		// reactorģʽ�����߳���ͨ��epoller��dealListen��ʵ�ֿͻ������ӵĽ����ͷַ��������read��writeϵͳ���������߳����(dealread & dealwrite)������������������
		// timeMS��ʾ������ô��ʱ��Wait�᷵���ж��ٸ��¼�
		int event_cnt = epoller_->wait(time_ms);
		for (int i = 0; i < event_cnt; i++) {
			int fd = epoller_->getEventFd(i);
			uint32_t events = epoller_->getEvent(i);
			if (fd == listen_fd_) {
				// ��������
				dealListen();
			}
			// ������http���Ӵ���
			// �����̴߳�����http���Ӻ���epoll�ں��¼�����ע���socket��д�����¼�������epoll_wait�ȴ�socket��д����socket��д����socketд�봦��ͻ�����Ľ��
			// �û����������ĺ��Ӧ��fd���пɶ��¼�
			else if (events & EPOLLIN) {
				dealRead(&users_[fd]);
			}
			else if (events & EPOLLOUT) {
				dealWrite(&users_[fd]);
			}
			// & (| | |)����д����ʾֻ�к��������¼�������һ����eventƥ�����
			// & �� | ��ʾλ���㣬������ͬ������λ�����򷵻ط������
			else if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
				closeConn(&users_[fd]);
			}
			else {
				LOG_ERROR("unexpected event");
			}
		}

	}
}

