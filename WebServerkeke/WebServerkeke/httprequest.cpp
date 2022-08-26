
// ���˶�Ӧ��ͷ�ļ����⣬�������ļ���ò�Ҫ���룬��Ȼ���ܻᵼ���ظ�����ͷ�ļ�
#include "httprequest.h"


#include <algorithm>
#include <regex>
// ��װ��mysql�������ͷ�ļ�
#include <mysql/mysql.h>
#include "sqlconnectionpool.h"
#include "sqlconnectionpoolRAII.h"



const std::set<std::string> HttpRequest::DEFAULT_HTML = {
	"/index", "/register", "/login", "/welcome", "/video",  "/picture",
};

HttpRequest::HttpRequest() {
	init();
}

void HttpRequest::init() {

	state_ = REQUEST_LINE;
	method_ = path_ = version_ = body_ = "";
	header_.clear();
	post_.clear();
}

bool HttpRequest::parse(Buffer& buff) {

	// tcpճ������Ҫ���ݽ����ַ�����
	// http����ÿһ�е�������\r\n��Ϊ�����ַ������������Ϊ�зֱ��
	const char end_singal[] = "\r\n";
	
	
	while (buff.readableBytes() && state_ != FINISH) {
		// ������algorithm�е�searchҲҪstd::
		const char* line_end = std::search(buff.beginReadPtr(), buff.beginWritePtr(), end_singal, end_singal + 2);
		
		std::string line(buff.beginReadPtr(), line_end);

		switch (state_)
		{
		// ���������״̬������������
		case REQUEST_LINE:
			parseRequestLine(line);
			parsePath();
			break;
		case HEADER:
			parseHeader(line);
			// ����parseHeader���state��Ϊbody�������ʣ�µ�ȫΪ���У���get���ģ�ֱ����ɽ���������ѭ��
			if (buff.readableBytes() <= 2) {
				state_ = FINISH;
			}
			break;
		case BODY:
			parseBody(line);
			break;
		}
		// line_end - buff.beginReadPtr()��ʾline�ĳ��ȣ�+2��\n�ĳ���
		buff.retrieve(line_end - buff.beginReadPtr() + 2);
	
	}
	return true;

	
}

void HttpRequest::parseRequestLine(std::string line) {
	std::regex pattern("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
	std::smatch sub_match;
	// ��׼����ҲҪ�������ռ�
	if (std::regex_match(line, sub_match, pattern)) {
		// http�����а����������͡�Ҫ���ʵ���Դ·����ʹ�õ�http�汾
		method_ = sub_match[1];
		path_ = sub_match[2];
		version_ = sub_match[3];
		state_ = HEADER;
	}
}

// ���������е�pathת��Ϊ����������Ӧ����Դ·��
void HttpRequest::parsePath() {
	if (path_ == "/") {
		path_ = "/index.html";
	}
	else {
		for (auto item : DEFAULT_HTML) {
			if (item == path_) {
				path_ += ".html";
				break;
			}
		}
	}
}
	

void HttpRequest::parseHeader(std::string line) {
	// header����:������ð��ǰ�����key���������value
	std::regex pattern("^([^:]*): ?(.*)$");
	std::smatch sub_match;
	if (std::regex_match(line, sub_match, pattern)) {
		// ���parseHeader�ᱻѭ������������header�е�ÿһ�У�ֱ���޷�ƥ�����޸�״̬ΪBody
		// ����header�е�key-value��
		header_[sub_match[1]] = sub_match[2];
	}
	else {
		state_ = BODY;
	}

}

void HttpRequest::parseBody(std::string line) {
	body_ = line;
	// ����Post�����е�body����
	if (method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {
		// post�ύ����ʱ�����ݸ�ʽ������ֻ������application/x-www-form-urlencoded�ĸ�ʽ�������ݣ������ʽֻ����ύ�ı�����
		// todo������body��, parseFromUrlEncoded()
		if (path_ == "/register.html" || path_ == "/login.html") {
			bool is_login;
			is_login = (path_ == "/login.html") ? true : false;
			// û�б����¼״̬��������·�post������Ҫ����У��
			if (userVerify(post_["username"], post_["password"], is_login)) {
				// �޸���ת��path
				path_ = "/welcome.html";
			}
			else {
				path_ = "/error.html";
			}
		}
	}
	state_ = FINISH;
}

bool HttpRequest::userVerify(std::string user_name, std::string password, bool is_login) {
	
	// ������mysql.h�У�linux���и��ļ�
	MYSQL* sql;
	// ����Instanceֻ���õ��Ѿ�����õ�ʵ����ʵ����ʼ���ڳ�ʼ��webserver��ʱ�����
	// RAII���õ�һ��������sql
	// ���ﲻӦ���� SqlConnectionPoolRAII temp(&sql, SqlConnectionPool::Instance())��ͨ��temp.sql_�����������
	SqlConnectionPoolRAII(&sql, SqlConnectionPool::Instance());

	// �洢sql����
	char order[256] = { 0 };
	std::snprintf(order, 256, "SELECT username, password FROM user where username='%s' LIMIT 1", user_name.c_str());
	// �洢sql���
	MYSQL_RES* res = nullptr;

	if (!mysql_query(sql, order)) {
		mysql_free_result(res);
		return false;
	}
	res = mysql_store_result(sql);

	// while����˵�����������ݣ�����user���Ѿ�ע���¼����
	while (MYSQL row = mysql_fetch_row(res)) {
		if (is_login && password == row[1]) {
			// ����ɹ�
			return true;
		}
		else {
			// ���������߷ǵ�¼״̬
			return false;
		}
	}

	// res��û���ݣ���ע��
	if (!is_login) {
		bzero(order, 256);
		std::snprintf(order, 256, "INSERT INTO user(username, password) VALUES('%s', '%s')", user_name.c_str(), password.c_str());
		if (!mysql_query(sql, order)) {
			// ע��ʧ��
			return false;
		}
	}
	SqlConnectionPool::Instance()->freeConn(sql);
	return true;
}

bool HttpRequest::isKeepAlive() {
	// header���������Connection:keep-alive�Ұ汾��1.1����keep-alive
	if (header_.count("Connection") == 1) {
		return header_.find("Connection")->second == "keep-alive" && version == "1.1";
	}
	return false;
}

std::string HttpRequest::path() {
	return path_;
}