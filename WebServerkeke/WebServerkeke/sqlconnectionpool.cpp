#include "sqlconnectionpool.h"

#include <mysql/mysql.h>


// 静态成员函数定义，不需要static，声明时static就可以了
SqlConnectionPool* SqlConnectionPool::Instance() {
	
	// 饿汉式，是一个静态局部变量,存储在全局数据区，第一次访问到这个声明时会构造这个static变量（默认初始化，之后init函数会对变量重新初始化）
	// 静态局部变量保证了每次访问instance()不会重新声明，都是得到这个对象
	// c++11保证如果多个线程同时初始化同一静态局部对象，初始化只发生一次，且这里初始化是系统默认构造（如果提供了构造则是人为的默认构造）
	// 这是定义了个静态局部变量，如果定义的是静态全局变量，那么静态全局变量必须在实现文件中定义，如定义在头文件中，那么每一个包含了该头文件的实现文件都有一个各自的静态全局变量
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
	// 初始化信号量
	sem_init(&sem_id_, 0, max_connect_);

}

// 当main执行完时，创建的static变量会析构，需要提供析构函数
SqlConnectionPool::~SqlConnectionPool() {
	closePool();
}

void SqlConnectionPool::closePool() {
	// 对队列操作都需要锁
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
	// 通过信号量保证队列中还有sql连接
	sem_wait(&sem_id_);
	{
		// 通过互斥锁对队列进行访问
		// lock_guard在构造对象的时候自动加锁，在离开作用域自动解锁
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



