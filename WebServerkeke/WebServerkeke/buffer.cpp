#include "buffer.h"
// 提供iovec
#include <sys/uio.h> 

// 多尝试用列表初始化
Buffer::Buffer(int init_size) : buffer_(init_size), read_pos_(0), write_pos_(0) {}

Buffer::~Buffer() {
	buffer_.clear();
	read_pos_ = 0;
	write_pos_ = 0;
}

size_t Buffer::readableBytes() const {
	return write_pos_ - read_pos_;
}

size_t Buffer::writeableBytes() const {
	return buffer_.size() - write_pos_;
}

size_t Buffer::prependableBytes() const {
	return read_pos_;
}

void Buffer::append(const char* str, int len) {
	if (writeableBytes() < len) {
		makeSpace(len);
	}
	// str用的是char*，因此copy的位置也要是char*
	std::copy(str, str + len, beginWritePtr());
	write_pos_ += len;
}

void Buffer::append(const std::string str) {
	append(str.data(), str.length());
}

// 保证还能写len的数据
void Buffer::makeSpace(int len) {
	// 如果后面的空间和前面的空间加起来都不够的话，扩容
	if (writeableBytes() + prependableBytes() < len) {
		buffer_.resize(write_pos_ + len + 1);
	}
	else {
		// 利用上前面的空间，将数据移到前面
		size_t readable = readableBytes();
		std::copy(beginReadPtr(), beginWritePtr(), &*buffer_.begin());
		read_pos_ = 0;
		write_pos_ = readable;
	}
}

// 读取数据后回收空间
void Buffer::retrieve(size_t len) {
	read_pos_ += len;
}

void Buffer::retrieveAll() {
	buffer_.clear();
	read_pos_ = 0;
	write_pos_ = 0;
}

const char* Buffer::beginReadPtr() {
	return &*buffer_.begin() + read_pos_;
}
char* Buffer::beginWritePtrNoConst() {
	return &*buffer_.begin() + write_pos_;
}

const char* Buffer::beginWritePtr() {
	return &*buffer_.begin() + write_pos_;
}

void Buffer::hasWritten(int n) {
	// 对类私有变量的操作最好都包装成方法的形式，也可以供类外调用
	write_pos_ += n;
}

// 从外部fd读数据到server（这里是读到iov[0]和iov[1]表示的内存中）
int Buffer::readFd(int fd, int* erreo) {
	char buff[65535];
	struct iovec iov[2];
	const size_t writeable = writeableBytes();

	// &*begin()返回的是数组首元素的指针
	// iov[0]是buffer类本身维护的vector
	iov[0].iov_base = beginWritePtr();
	iov[0].iov_len = writeable;

	// 数组名代表数组首元素的地址，sizeof求数组长度
	// iov[1]为临时定义的char数组
	iov[1].iov_base = buff;
	iov[1].iov_len = sizeof(buff);

	// readv定义在uio.h中，分散读，将数据从fd读到分散的内存块中，writev将分散数据一并发送，减少io调用
	// read的时候是先分散读，但最后都写到buff里
	// fd代表的是一个打开的文件，打开的文件已经在内存中了,通过对fd操作，即可以对相应的文件操作
	// 调用这个函数的时候，数据已经读到了相应的文件中
	// 非阻塞io，程序不阻塞在io系统调用上，而是阻塞在epoll_wait这种地方



	// readv系统调用：可能会阻塞在数据从网卡到内核缓冲区和从内核缓冲区到用户缓冲区两个地方
	// 非阻塞IO是将socket的readv的从网卡到内核缓冲区这一步设置为非阻塞（即会返回值，如果返回-1就说明没有数据，重复readv），第二步一般都阻塞且同步
	// 但本项目中用的是IO多路复用而非非阻塞IO，所以从网卡到内核这里交给epoll即可，下面这行代码则是从内核拷贝到用户缓冲区的实现
	const size_t len = readv(fd, iov, 2);
	// readv已经将数据读到iov里面了，所以只需要移动pos
	if (len < writeable) {
		// 数据全部存到第一块缓冲区也就是buffer_中，移动pos
		write_pos_ += len;
	}
	else {
		// 读取的字节超过buffer，扩容，最后的数据还是全部读到了buffer中
		write_pos_ = buffer_.size();
		append(buff, len - writeable);
	}
	return len;
}