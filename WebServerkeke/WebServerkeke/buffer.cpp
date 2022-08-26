#include "buffer.h"
// �ṩiovec
#include <sys/uio.h> 

// �ೢ�����б��ʼ��
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
	// str�õ���char*�����copy��λ��ҲҪ��char*
	std::copy(str, str + len, beginWritePtr());
	write_pos_ += len;
}

void Buffer::append(const std::string str) {
	append(str.data(), str.length());
}

// ��֤����дlen������
void Buffer::makeSpace(int len) {
	// �������Ŀռ��ǰ��Ŀռ�������������Ļ�������
	if (writeableBytes() + prependableBytes() < len) {
		buffer_.resize(write_pos_ + len + 1);
	}
	else {
		// ������ǰ��Ŀռ䣬�������Ƶ�ǰ��
		size_t readable = readableBytes();
		std::copy(beginReadPtr(), beginWritePtr(), &*buffer_.begin());
		read_pos_ = 0;
		write_pos_ = readable;
	}
}

// ��ȡ���ݺ���տռ�
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
	// ����˽�б����Ĳ�����ö���װ�ɷ�������ʽ��Ҳ���Թ��������
	write_pos_ += n;
}

// ���ⲿfd�����ݵ�server�������Ƕ���iov[0]��iov[1]��ʾ���ڴ��У�
int Buffer::readFd(int fd, int* erreo) {
	char buff[65535];
	struct iovec iov[2];
	const size_t writeable = writeableBytes();

	// &*begin()���ص���������Ԫ�ص�ָ��
	// iov[0]��buffer�౾��ά����vector
	iov[0].iov_base = beginWritePtr();
	iov[0].iov_len = writeable;

	// ����������������Ԫ�صĵ�ַ��sizeof�����鳤��
	// iov[1]Ϊ��ʱ�����char����
	iov[1].iov_base = buff;
	iov[1].iov_len = sizeof(buff);

	// readv������uio.h�У���ɢ���������ݴ�fd������ɢ���ڴ���У�writev����ɢ����һ�����ͣ�����io����
	// read��ʱ�����ȷ�ɢ���������д��buff��
	// fd�������һ���򿪵��ļ����򿪵��ļ��Ѿ����ڴ�����,ͨ����fd�����������Զ���Ӧ���ļ�����
	// �������������ʱ�������Ѿ���������Ӧ���ļ���
	// ������io������������ioϵͳ�����ϣ�����������epoll_wait���ֵط�



	// readvϵͳ���ã����ܻ����������ݴ��������ں˻������ʹ��ں˻��������û������������ط�
	// ������IO�ǽ�socket��readv�Ĵ��������ں˻�������һ������Ϊ�����������᷵��ֵ���������-1��˵��û�����ݣ��ظ�readv�����ڶ���һ�㶼������ͬ��
	// ������Ŀ���õ���IO��·���ö��Ƿ�����IO�����Դ��������ں����ｻ��epoll���ɣ��������д������Ǵ��ں˿������û���������ʵ��
	const size_t len = readv(fd, iov, 2);
	// readv�Ѿ������ݶ���iov�����ˣ�����ֻ��Ҫ�ƶ�pos
	if (len < writeable) {
		// ����ȫ���浽��һ�黺����Ҳ����buffer_�У��ƶ�pos
		write_pos_ += len;
	}
	else {
		// ��ȡ���ֽڳ���buffer�����ݣ��������ݻ���ȫ��������buffer��
		write_pos_ = buffer_.size();
		append(buff, len - writeable);
	}
	return len;
}