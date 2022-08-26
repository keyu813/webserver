#ifndef SQLCONNECTIONPOLL_H
#define SQLCONNECTIONPOLL_H

#include <mutex>
#include <mysql/mysql.h>
#include <queue>
#include <semaphore.h>

class SqlConnectionPool {

public:
	// 对外暴露的获取实例和销毁实例的方法，静态成员函数
	// 饿汉式
	static SqlConnectionPool* Instance();
	void init();
	void closePool();

	// 从连接池中获取连接以及释放连接
	MYSQL* getConn();
	void freeConn(MYSQL* conn);

private:
	// 单例模式，构造私有
	// 不需要禁用构造（刚访问静态局部变量的时候还需要用默认构造来初始化），只要禁用移动和拷贝即可
	// 析构私有保证对象只能在堆上生成（堆上对象构造析构时机程序员确定），无法在栈上生成（栈上的对象会自动析构，需要析构可访问）
	SqlConnectionPool() = default;
	~SqlConnectionPool();

	// 存放连接池中的连接
	std::queue<MYSQL*> conn_queue_;
	int max_connect_;


	std::mutex mtx_;
	// 存放信号量，作用上有点像条件变量，表示连接数目，但信号量是会维护一个值的
	// 信号量像是维护了数量的条件变量
	sem_t sem_id_;

};


#endif // !SQLCONNECTIONPOLL_H
