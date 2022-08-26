#ifndef SQLCONNECTIONPOOLRAII_H
#define SQLCONNECTIONPOOLRAII_H

#include <mysql/mysql.h>
#include "sqlconnectionpool.h"

/* RAIIģʽ����Դ�ڶ������ʼ�� ��Դ�ڶ�������ʱ�ͷ�*/
// �ڹ��캯����������Դ���������������ͷ���Դ��������������Դ
// ����Դ�Ͷ����������ڰ󶨣�ͨ��c++���Ի���ʵ����Դ�Ĺ�������ֶ�����
// ����ָ����RAII������
// Ҫʹ�����ݿ������ʹ��RAII����ӵ��ã�����Ҫһ�����Ӿʹ���һ��RAII����
class SqlConnectionPoolRAII {

public:
	SqlConnectionPoolRAII() = default;
	// ����RAII����ʱ��Ҫ����һ��pool���󣨵���ģʽ��ͨ��init()��ʼ���ˣ�
	SqlConnectionPoolRAII(MYSQL** sql, SqlConnectionPool* pool) {
		// �����ã��õ�һ�����Ӹ�ֵ�����Ա����sql_
		*sql = pool->getConn();
		sql_ = *sql;
		conn_pool_ = pool;
	}

	// ����ֻ���ͷŵ����Ӷ���û�ͷ�pool
	~SqlConnectionPoolRAII() {
		if (sql_) {
			conn_pool_->freeConn(sql_);
		}
	}

private:
	// ÿ������ά��һ��conn�����һ�����ӳض���
	MYSQL* sql_;
	SqlConnectionPool* conn_pool_;

};


#endif // !SQLCONNECTIONPOOLRAII_H
