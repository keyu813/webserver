#ifndef SQLCONNECTIONPOOLRAII_H
#define SQLCONNECTIONPOOLRAII_H

#include <mysql/mysql.h>
#include "sqlconnectionpool.h"

/* RAII模式，资源在对象构造初始化 资源在对象析构时释放*/
// 在构造函数中申请资源，在析构函数中释放资源，用类来管理资源
// 将资源和对象生命周期绑定，通过c++语言机制实现资源的管理避免手动操作
// 智能指针是RAII的例子
// 要使用数据库最后还是使用RAII来间接调用，即需要一个连接就创建一个RAII对象
class SqlConnectionPoolRAII {

public:
	SqlConnectionPoolRAII() = default;
	// 创建RAII对象时需要传入一个pool对象（单例模式，通过init()初始化了）
	SqlConnectionPoolRAII(MYSQL** sql, SqlConnectionPool* pool) {
		// 解引用，拿到一个连接赋值给类成员变量sql_
		*sql = pool->getConn();
		sql_ = *sql;
		conn_pool_ = pool;
	}

	// 析构只是释放掉连接对象，没释放pool
	~SqlConnectionPoolRAII() {
		if (sql_) {
			conn_pool_->freeConn(sql_);
		}
	}

private:
	// 每个对象维护一个conn对象和一个连接池对象
	MYSQL* sql_;
	SqlConnectionPool* conn_pool_;

};


#endif // !SQLCONNECTIONPOOLRAII_H
