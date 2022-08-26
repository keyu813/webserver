#ifndef HTTPRESPONSE_H
#define HTTPRESPONSE_H

#include "buffer.h"

#include <sys/stat.h>

class HttpResponse {

public:
	HttpResponse();
	~HttpResponse() = default;

	void init(std::string src_dir, std::string path, bool is_keep_alive, int code);
	void unmapFile();

	// 因为这里是对buff本身的操作，所以用引用/指针
	void makeResponse(Buffer& buff);

	void addStateLine(Buffer& buff);
	void addHeader(Buffer& buff);
	void addContent(Buffer& buff);

	void errorContent(Buffer& buff, std::string message);

	// 返回指向文件的指针
	char* file();
	int fileLen();

private:
	// 存放server资源的源地址
	std::string src_dir_;
	std::string path_;

	// 响应报文可能需要文件，用mm_file_表示，mmap方法将文件映射到内存
	char* mm_file_;
	// stat取得文件属性，struct包括文件类型权限st_mode和文件大小st_size
	struct stat mm_file_stat_;
	// 返回报文的码(eg 404)
	int code_;

	// 当开启keep_alive后，当前数据发送完后不会断开连接，否则数据发送完就会断开当前tcp连接
	bool is_keep_alive_;
}


#endif // !RESPONSE_H
