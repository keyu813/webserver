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

	// ��Ϊ�����Ƕ�buff����Ĳ���������������/ָ��
	void makeResponse(Buffer& buff);

	void addStateLine(Buffer& buff);
	void addHeader(Buffer& buff);
	void addContent(Buffer& buff);

	void errorContent(Buffer& buff, std::string message);

	// ����ָ���ļ���ָ��
	char* file();
	int fileLen();

private:
	// ���server��Դ��Դ��ַ
	std::string src_dir_;
	std::string path_;

	// ��Ӧ���Ŀ�����Ҫ�ļ�����mm_file_��ʾ��mmap�������ļ�ӳ�䵽�ڴ�
	char* mm_file_;
	// statȡ���ļ����ԣ�struct�����ļ�����Ȩ��st_mode���ļ���Сst_size
	struct stat mm_file_stat_;
	// ���ر��ĵ���(eg 404)
	int code_;

	// ������keep_alive�󣬵�ǰ���ݷ�����󲻻�Ͽ����ӣ��������ݷ�����ͻ�Ͽ���ǰtcp����
	bool is_keep_alive_;
}


#endif // !RESPONSE_H
