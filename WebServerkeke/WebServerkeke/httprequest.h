#ifndef HTTPREQUEST_H
#define HTTPREQUEST_H

#include "buffer.h"
#include <map>
#include <set>

class HttpRequest {

public:
	enum PARSE_STATE {
		REQUEST_LINE,
		HEADER,
		BODY,
		FINISH,
	};

	HttpRequest();
	~HttpRequest() = default;

	void init();
	bool parse(Buffer& buff);
	void parseRequestLine(std::string line);
	void parsePath();
	void parseHeader(std::string line);
	void parseBody(std::string line);

	// is_login区分是注册还是登录
	bool userVerify(std::string user_name, std::string password, bool login);

	bool isKeepAlive();

	std::string path();

private:
	PARSE_STATE state_;

	std::string method_, path_, version_, body_;
	std::map<std::string, std::string> header_;
	std::map<std::string, std::string> post_;

	// 存放server的所有html资源
	static const std::set<std::string> DEFAULT_HTML;
};


#endif // !HTTPREQUEST_H
