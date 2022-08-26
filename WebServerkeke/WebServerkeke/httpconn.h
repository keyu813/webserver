#ifndef HTTPCONN_H
#define HTTPCONN_H

#include <atomic>
#include <winsock.h>

#include "buffer.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConn {

public:
	// 构造里面如果没有显示调用Buffer的构造函数则调用无参构造
	HttpConn() = default;
	~HttpConn();

	void init(int fd, sockaddr_in addr);

	int getFd();

	int read(int* error);

	int write(int* error);

	bool process();

	bool toWriteByte();

	// static定义为public供外部访问，根据webserver_event是否为ET来确定httpconn是否为ET方式
	// static成员变量只能在类内部声明，不能定义，因为定义要分配空间而类这里没有空间（静态常量整型数据成员可以类中初始化）
	static bool is_ET_;
	// 由于不同线程中每个类对象都会操作user_count_，需要atomic
	static std::atomic<int> user_count_;

	// 存放资源源目录
	static std::string src_dir_;

	bool isKeepAlive();

private:
	
	// 每一个用户http连接，在服务器上都只是对应为一个fd和对应的打开文件，读写数据都是服务器本身和这个fd之间通信
	int fd_;
	struct sockaddr_in addr_;

	// 每个连接需要维护一个读写缓冲区，数据都是到缓冲区中再到浏览器端
	// 读缓冲区缓存了请求报文，http连接请求的所有数据都在读缓冲区中
	Buffer read_buff_;
	// 缓存响应报文
	Buffer write_buff_;

	HttpRequest http_request_;
	HttpResponse http_response_;

	// 维护的iov_主要是用于存储要发送的数据
	struct iovec iov_[2];
	int iov_count_;




};



#endif // !HTTPCONN_H
