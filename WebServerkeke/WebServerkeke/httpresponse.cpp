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

// ȡ�����ļ�ӳ�䵽�ڴ�
void HttpResponse::unmapFile() {
	if (mm_file_) {
		munmap(mm_file_, mm_file_stat_.st_size);
		mm_file_ = nullptr;
	}
}

void HttpResponse::makeResponse(Buffer& buff) {
	// stat()����ȡ���ļ����Բ�����mm_file_stat��
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

// contentָ�ľ��ǿͻ��������file
void HttpResponse::addContent(Buffer& buff) {
	// open��fnctl.h��
	int src_fd = open((src_dir_ + path_).data(), O_RDONLY);
	if (src_fd < 0) {
		// ���ش���ҳ��
		errorContent(buff, "File Not Found");
		return;
	}
	// ���ļ�ӳ�䵽�ڴ�����߷����ٶȣ�mmap��mman.h��
	// st_size��ʾ������ӳ�����ĳ���
	// MAP_PRIVATE��ʾ�ڴ������д�벻��Ӱ�쵽Դ�ļ�
	int* mm_ret = (int*)mmap(0, mm_file_stat_.st_size, PORT_READ, MAP_PRIVATE, src_fd, 0);
	// mm_file_Ϊӳ�䵽���ڴ棬��ָ������ļ��ľ���html����
	mm_file_ = (char*)mm_ret;
	// close��unist.h��
	close(src_fd);
	buff.append("content-length" + std::to_string(mm_file_stat_.st_size) + "\r\n\r\n");
}

// �������ҳ��
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
