
// 除了对应的头文件以外，其它的文件最好不要引入，不然可能会导致重复引入头文件
#include "httprequest.h"


#include <algorithm>
#include <regex>
// 得装了mysql才有这个头文件
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

	// tcp粘包，需要根据结束字符区分
	// http报文每一行的数据用\r\n作为结束字符，按照这个作为切分标记
	const char end_singal[] = "\r\n";
	
	
	while (buff.readableBytes() && state_ != FINISH) {
		// 定义在algorithm中的search也要std::
		const char* line_end = std::search(buff.beginReadPtr(), buff.beginWritePtr(), end_singal, end_singal + 2);
		
		std::string line(buff.beginReadPtr(), line_end);

		switch (state_)
		{
		// 利用正则和状态机来解析报文
		case REQUEST_LINE:
			parseRequestLine(line);
			parsePath();
			break;
		case HEADER:
			parseHeader(line);
			// 本来parseHeader完后state变为body，但如果剩下的全为空行，是get报文，直接完成解析，跳出循环
			if (buff.readableBytes() <= 2) {
				state_ = FINISH;
			}
			break;
		case BODY:
			parseBody(line);
			break;
		}
		// line_end - buff.beginReadPtr()表示line的长度，+2是\n的长度
		buff.retrieve(line_end - buff.beginReadPtr() + 2);
	
	}
	return true;

	
}

void HttpRequest::parseRequestLine(std::string line) {
	std::regex pattern("^([^ ]*) ([^ ]*) HTTP/([^ ]*)$");
	std::smatch sub_match;
	// 标准方法也要加命名空间
	if (std::regex_match(line, sub_match, pattern)) {
		// http请求行包括请求类型、要访问的资源路径、使用的http版本
		method_ = sub_match[1];
		path_ = sub_match[2];
		version_ = sub_match[3];
		state_ = HEADER;
	}
}

// 将请求行中的path转变为服务器上相应的资源路径
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
	// header按照:解析，冒号前面的是key，后面的是value
	std::regex pattern("^([^:]*): ?(.*)$");
	std::smatch sub_match;
	if (std::regex_match(line, sub_match, pattern)) {
		// 这个parseHeader会被循环调用来解析header中的每一行，直到无法匹配再修改状态为Body
		// 构建header中的key-value对
		header_[sub_match[1]] = sub_match[2];
	}
	else {
		state_ = BODY;
	}

}

void HttpRequest::parseBody(std::string line) {
	body_ = line;
	// 解析Post报文中的body部分
	if (method_ == "POST" && header_["Content-Type"] == "application/x-www-form-urlencoded") {
		// post提交报文时的数据格式，这里只考虑以application/x-www-form-urlencoded的格式传输数据，这个格式只针对提交的表单数据
		// todo：解析body体, parseFromUrlEncoded()
		if (path_ == "/register.html" || path_ == "/login.html") {
			bool is_login;
			is_login = (path_ == "/login.html") ? true : false;
			// 没有保存登录状态，如果重新发post请求需要重新校验
			if (userVerify(post_["username"], post_["password"], is_login)) {
				// 修改跳转的path
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
	
	// 定义在mysql.h中，linux下有该文件
	MYSQL* sql;
	// 这里Instance只是拿到已经构造好的实例，实例初始化在初始化webserver的时候完成
	// RAII，拿到一个新连接sql
	// 这里不应该是 SqlConnectionPoolRAII temp(&sql, SqlConnectionPool::Instance())再通过temp.sql_来获得连接吗？
	SqlConnectionPoolRAII(&sql, SqlConnectionPool::Instance());

	// 存储sql命令
	char order[256] = { 0 };
	std::snprintf(order, 256, "SELECT username, password FROM user where username='%s' LIMIT 1", user_name.c_str());
	// 存储sql结果
	MYSQL_RES* res = nullptr;

	if (!mysql_query(sql, order)) {
		mysql_free_result(res);
		return false;
	}
	res = mysql_store_result(sql);

	// while成立说明里面有数据，即该user是已经注册登录过的
	while (MYSQL row = mysql_fetch_row(res)) {
		if (is_login && password == row[1]) {
			// 核验成功
			return true;
		}
		else {
			// 密码错误或者非登录状态
			return false;
		}
	}

	// res中没数据，是注册
	if (!is_login) {
		bzero(order, 256);
		std::snprintf(order, 256, "INSERT INTO user(username, password) VALUES('%s', '%s')", user_name.c_str(), password.c_str());
		if (!mysql_query(sql, order)) {
			// 注册失败
			return false;
		}
	}
	SqlConnectionPool::Instance()->freeConn(sql);
	return true;
}

bool HttpRequest::isKeepAlive() {
	// header里面如果有Connection:keep-alive且版本是1.1则是keep-alive
	if (header_.count("Connection") == 1) {
		return header_.find("Connection")->second == "keep-alive" && version == "1.1";
	}
	return false;
}

std::string HttpRequest::path() {
	return path_;
}