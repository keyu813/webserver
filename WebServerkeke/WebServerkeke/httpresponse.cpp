#include "httpresponse.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>



HttpResponse::HttpResponse() {
	init("", "", false, -1);
}

void HttpResponse::init(std::string src_dir, std::string path, bool is_keep_alive, int code) {
	if (mm_file_) {
		unmapFile();
	}
	src_dir_ = src_dir;
	path_ = path;
	is_keep_alive_ = is_keep_alive;
	code_ = code;
	mm_file_ = nullptr;
	mm_file_stat_ = { 0 };
}

// 取消将文件映射到内存
void HttpResponse::unmapFile() {
	if (mm_file_) {
		munmap(mm_file_, mm_file_stat_.st_size);
		mm_file_ = nullptr;
	}
}

void HttpResponse::makeResponse(Buffer& buff) {
	// stat()方法取得文件属性并放在mm_file_stat中
	if (stat((src_dir_ + path_).data(), &mm_file_stat_) < 0) {
		code_ = 404;
	}
	addStateLine(buff);
	addHeader(buff);
	addContent(buff);
}

void HttpResponse::addStateLine(Buffer& buff) {
	std::string status;
	buff.append("HTTP/1.1 " + std::to_string(code_) + " " + status + "\r\n");
}

void HttpResponse::addHeader(Buffer& buff) {
	buff.append("Connection: ");
	if (is_keep_alive_) {
		buff.append("keep-alive\r\n");
		buff.append("keep-alive: max=6, timeout=120\r\n");
	}
	else {
		buff.append("close\r\n");
	}


}

// content指的就是客户端请求的file
void HttpResponse::addContent(Buffer& buff) {
	// open在fnctl.h中
	int src_fd = open((src_dir_ + path_).data(), O_RDONLY);
	if (src_fd < 0) {
		// 返回错误页面
		errorContent(buff, "File Not Found");
		return;
	}
	// 将文件映射到内存中提高访问速度，mmap在mman.h中
	// st_size表示期望的映射区的长度
	// MAP_PRIVATE表示内存区域的写入不会影响到源文件
	int* mm_ret = (int*)mmap(0, mm_file_stat_.st_size, PORT_READ, MAP_PRIVATE, src_fd, 0);
	// mm_file_为映射到的内存，其指向的是文件的具体html代码
	mm_file_ = (char*)mm_ret;
	// close在unist.h中
	close(src_fd);
	buff.append("content-length" + std::to_string(mm_file_stat_.st_size) + "\r\n\r\n");
}

// 构造错误页面
void HttpResponse::errorContent(Buffer& buff, std::string message) {
	std::string body;
	std::string status = "Bad Request";
	body += "<html><title>Error</title>";
	body += std::to_string(code_) + " : " + status + "\n";
	body += "<p>" + message + "<p>";

	buff.append("content-length: " + std::to_string(body.size()) + "\r\n\r\n");
	buff.append(body);
}

char* HttpResponse::file() {
	return mm_file_;
}

int HttpResponse::fileLen() {
	return mm_file_stat_.st_size;
}
