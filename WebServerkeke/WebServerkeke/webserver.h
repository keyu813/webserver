// ���û�к궨�壬��ô��define���ұ���webserver��
#ifndef WEBSERVER_H

#define WEBSERVER_H

#include <map>
// ����ָ��
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

	// ����ֻ�ṩ��start������server�����������������˽��
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
	// ���Źر�����, false��ʹ��
	// �������������Ա������һ����ʼֵ�����ڳ�ʼ��
	bool opt_linger_;
	int sql_num_;
	int thread_num_;
	bool close_log_;
	int actor_mode_;

	bool server_close_;

	int listen_fd_;
	// ���fd, httpconn��ӳ��
	std::map<int, HttpConn> users_;

	std::unique_ptr<ThreadPool> thread_pool_;

	std::unique_ptr<Epoller> epoller_;
	
	std::unique_ptr<Timer> timer_;


	// ��ʾwebserver�ڴ���������������¼�ʱ����ʲôģʽ
	uint32_t listen_event_;
	uint32_t conn_event_;

	// server����ܽ��ն��ٸ�http conn
	static const int max_fd_ = 65535;


};

#endif // !WEBSERVER_H
