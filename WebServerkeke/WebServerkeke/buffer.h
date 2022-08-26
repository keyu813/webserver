#ifndef BUFFER_H
#define BUFFER_H

#include <string>
#include <vector>
#include <atomic>

class Buffer {

public:
	Buffer(int init_size = 1024);
	~Buffer();

	size_t readableBytes() const;
	size_t writeableBytes() const;
	// ��Ҫ��read_pos_ǰ����һ����
	size_t prependableBytes() const;

	// ��buffer�м�Ԫ�أ��������str��ǰlen�ĳ���
	void append(const char* str, int len);
	void append(const std::string str);
	
	void makeSpace(int len);

	// ����buffer
	void retrieve(size_t len);
	void retrieveAll();

	const char* beginReadPtr();
	char* beginWritePtrNoConst();
	const char* beginWritePtr();

	void hasWritten(int n);

	int readFd(int fd, int* error);

private:
	std::vector<char> buffer_;

	// httpconnά������buffer��һ��������һ������д����ÿ��buffer��Ҫά�����˺�д��
	std::size_t read_pos_;
	std::size_t write_pos_;

};

#endif // !BUFFER_H