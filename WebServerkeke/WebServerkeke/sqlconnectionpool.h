#ifndef SQLCONNECTIONPOLL_H
#define SQLCONNECTIONPOLL_H

#include <mutex>
#include <mysql/mysql.h>
#include <queue>
#include <semaphore.h>

class SqlConnectionPool {

public:
	// ���Ⱪ¶�Ļ�ȡʵ��������ʵ���ķ�������̬��Ա����
	// ����ʽ
	static SqlConnectionPool* Instance();
	void init();
	void closePool();

	// �����ӳ��л�ȡ�����Լ��ͷ�����
	MYSQL* getConn();
	void freeConn(MYSQL* conn);

private:
	// ����ģʽ������˽��
	// ����Ҫ���ù��죨�շ��ʾ�̬�ֲ�������ʱ����Ҫ��Ĭ�Ϲ�������ʼ������ֻҪ�����ƶ��Ϳ�������
	// ����˽�б�֤����ֻ���ڶ������ɣ����϶���������ʱ������Աȷ�������޷���ջ�����ɣ�ջ�ϵĶ�����Զ���������Ҫ�����ɷ��ʣ�
	SqlConnectionPool() = default;
	~SqlConnectionPool();

	// ������ӳ��е�����
	std::queue<MYSQL*> conn_queue_;
	int max_connect_;


	std::mutex mtx_;
	// ����ź������������е���������������ʾ������Ŀ�����ź����ǻ�ά��һ��ֵ��
	// �ź�������ά������������������
	sem_t sem_id_;

};


#endif // !SQLCONNECTIONPOLL_H
