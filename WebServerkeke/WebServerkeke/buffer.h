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
	// 主要是read_pos_前面那一部分
	size_t prependableBytes() const;

	// 往buffer中加元素，插入的是str中前len的长度
	void append(const char* str, int len);
	void append(const std::string str);
	
	void makeSpace(int len);

	// 回收buffer
	void retrieve(size_t len);
	void retrieveAll();

	const char* beginReadPtr();
	char* beginWritePtrNoConst();
	const char* beginWritePtr();

	void hasWritten(int n);

	int readFd(int fd, int* error);

private:
	std::vector<char> buffer_;

	// httpconn维护两个buffer（一个用来读一个用来写），每个buffer都要维护读端和写端
	std::size_t read_pos_;
	std::size_t write_pos_;

};

#endif // !BUFFER_H