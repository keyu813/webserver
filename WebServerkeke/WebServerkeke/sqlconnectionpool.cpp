#include "sqlconnectionpool.h"

#include <mysql/mysql.h>


// ��̬��Ա�������壬����Ҫstatic������ʱstatic�Ϳ�����
SqlConnectionPool* SqlConnectionPool::Instance() {
	
	// ����ʽ����һ����̬�ֲ�����,�洢��ȫ������������һ�η��ʵ��������ʱ�ṹ�����static������Ĭ�ϳ�ʼ����֮��init������Ա������³�ʼ����
	// ��̬�ֲ�������֤��ÿ�η���instance()�����������������ǵõ��������
	// c++11��֤�������߳�ͬʱ��ʼ��ͬһ��̬�ֲ����󣬳�ʼ��ֻ����һ�Σ��������ʼ����ϵͳĬ�Ϲ��죨����ṩ�˹���������Ϊ��Ĭ�Ϲ��죩
	// ���Ƕ����˸���̬�ֲ����������������Ǿ�̬ȫ�ֱ�������ô��̬ȫ�ֱ���������ʵ���ļ��ж��壬�綨����ͷ�ļ��У���ôÿһ�������˸�ͷ�ļ���ʵ���ļ�����һ�����Եľ�̬ȫ�ֱ���
	static SqlConnectionPool connection_pool;
	return &connection_pool;
}


void SqlConnectionPool::init() {
	char* host;
	int port;
	char* user;
	char* pwd;
	char* db_name;
	int conn_size = 10;
	for (int i = 0; i < conn_size; i++) {
		MYSQL* sql = nullptr;
		sql = mysql_init(sql);
		sql = mysql_real_connect(sql, host, user, pwd, db_name, port, nullptr, 0);
		conn_queue_.push(sql);
	}
	max_connect_ = conn_size;
	// ��ʼ���ź���
	sem_init(&sem_id_, 0, max_connect_);

}

// ��mainִ����ʱ��������static��������������Ҫ�ṩ��������
SqlConnectionPool::~SqlConnectionPool() {
	closePool();
}

void SqlConnectionPool::closePool() {
	// �Զ��в�������Ҫ��
	std::lock_guard<std::mutex> locker(mtx_);
	while (!conn_queue_.empty()) {
		auto item = conn_queue_.front();
		conn_queue_.pop();
		mysql_close(item);
	}
	mysql_library_end();

}

MYSQL* SqlConnectionPool::getConn() {
	
	MYSQL* conn = nullptr;
	// ͨ���ź�����֤�����л���sql����
	sem_wait(&sem_id_);
	{
		// ͨ���������Զ��н��з���
		// lock_guard�ڹ�������ʱ���Զ����������뿪�������Զ�����
		std::lock_guard<std::mutex> locker(mtx_);
		conn = conn_queue_.front();
		conn_queue_.pop();
	}
	return conn;
}

void SqlConnectionPool::freeConn(MYSQL* conn) {
	std::lock_guard<std::mutex> locker(mtx_);
	conn_queue_.push(conn);
	sem_post(&sem_id_);	
}



