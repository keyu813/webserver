#ifndef HTTPCONN_H
#define HTTPCONN_H

#include <atomic>
#include <winsock.h>

#include "buffer.h"
#include "httprequest.h"
#include "httpresponse.h"

class HttpConn {

public:
	// �����������û����ʾ����Buffer�Ĺ��캯��������޲ι���
	HttpConn() = default;
	~HttpConn();

	void init(int fd, sockaddr_in addr);

	int getFd();

	int read(int* error);

	int write(int* error);

	bool process();

	bool toWriteByte();

	// static����Ϊpublic���ⲿ���ʣ�����webserver_event�Ƿ�ΪET��ȷ��httpconn�Ƿ�ΪET��ʽ
	// static��Ա����ֻ�������ڲ����������ܶ��壬��Ϊ����Ҫ����ռ��������û�пռ䣨��̬�����������ݳ�Ա�������г�ʼ����
	static bool is_ET_;
	// ���ڲ�ͬ�߳���ÿ������󶼻����user_count_����Ҫatomic
	static std::atomic<int> user_count_;

	// �����ԴԴĿ¼
	static std::string src_dir_;

	bool isKeepAlive();

private:
	
	// ÿһ���û�http���ӣ��ڷ������϶�ֻ�Ƕ�ӦΪһ��fd�Ͷ�Ӧ�Ĵ��ļ�����д���ݶ��Ƿ�������������fd֮��ͨ��
	int fd_;
	struct sockaddr_in addr_;

	// ÿ��������Ҫά��һ����д�����������ݶ��ǵ����������ٵ��������
	// �������������������ģ�http����������������ݶ��ڶ���������
	Buffer read_buff_;
	// ������Ӧ����
	Buffer write_buff_;

	HttpRequest http_request_;
	HttpResponse http_response_;

	// ά����iov_��Ҫ�����ڴ洢Ҫ���͵�����
	struct iovec iov_[2];
	int iov_count_;




};



#endif // !HTTPCONN_H
