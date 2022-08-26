// 如果没有宏定义，那么就define并且编译webserver类
#ifndef WEBSERVER_H

#define WEBSERVER_H

#include <map>
// 智能指针
#include <memory>



#include "httpconn.h"
#include "epoller.h"
#include "threadpool.h"
#include "timer.h"

class WebServer
{
public:
	WebServer() = default;
	WebServer(int port, bool syncLogWrite, int triggerMode, bool optLinger, int sqlNum, int threadNum, bool closeLog, int acterMode);
	~WebServer();

	// 这里只提供了start方法给server，因此其它方法都是私有
	void start();

private:

	void initEventMode(int trigger_mode);
	void initListenFd();

	void setFdNonBlock(int fd);

	void extendTime(HttpConn* client);

	void dealListen();
	void dealRead(HttpConn* client);
	void onRead(HttpConn* client);
	void dealWrite(HttpConn* client);
	void onWrite(HttpConn* client);
	void closeConn(HttpConn* client);


	void onProcess(HttpConn* client);


	// cli param
	int port_;
	bool sync_log_write_;
	int trigger_mode_;
	// 优雅关闭连接, false不使用
	// 可以在这里给成员变量赋一个初始值，类内初始化
	bool opt_linger_;
	int sql_num_;
	int thread_num_;
	bool close_log_;
	int actor_mode_;

	bool server_close_;

	int listen_fd_;
	// 存放fd, httpconn的映射
	std::map<int, HttpConn> users_;

	std::unique_ptr<ThreadPool> thread_pool_;

	std::unique_ptr<Epoller> epoller_;
	
	std::unique_ptr<Timer> timer_;


	// 表示webserver在处理监听或者连接事件时采用什么模式
	uint32_t listen_event_;
	uint32_t conn_event_;

	// server最多能接收多少个http conn
	static const int max_fd_ = 65535;


};

#endif // !WEBSERVER_H
