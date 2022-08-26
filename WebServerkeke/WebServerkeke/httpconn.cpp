#include "httpconn.h"



HttpConn::~HttpConn() {
	http_response_.unmapFile();
	user_count_--;
	close(fd_);
}
int HttpConn::getFd() {
	return fd_;
}

void HttpConn::init(int fd, sockaddr_in addr) {
	user_count_++;
	fd_ = fd;
	addr_ = addr;
	// 初始化缓冲区
	read_buff_.retrieveAll();
	write_buff_.retrieveAll();
}

// read是将数据读到读缓冲区中
int HttpConn::read(int* error) {
	size_t len = -1;
	do {
		// 数据读到read_buff中
		len = read_buff_.readFd(fd_, error);
	} while (is_ET_);
	return len;
}


// write是将数据从写缓冲（可能还有文件）写到浏览器端
// 这里已经完成了响应报文，主要是进行将响应报文写到浏览器端的逻辑
int HttpConn::write(int* error) {
	
	size_t len = -1;
	do {
		// writev这个系统调用一次写多个非连续缓冲区，顺序写到iov_[0],iov_[1]...中，返回的len应该是iov_[0]/iov_[1]...所有的值的和
		// writev是写到fd，即写到浏览器端
		// iov_[0]就是writebuff，在makeresponse中已经把内容写到writebuff里也即iov_了，再在这写到浏览器端
		// iov_的内容都在process函数中处理好了
		// reactor模式，writev系统调用可能会阻塞
		len = writev(fd_, iov_, iov_count_);
		// 传输结束
		if (iov_[0].iov_len + iov_[1].iov_len == 0) {
			break;
		}
		// 已经发送完iov_[0]中的数据，更新iov_[1]并且清除writebuff
		else if (static_cast<size_t>(len) > iov_[0].iov_len) {
			uint8_t iov1_len = len - iov_[0].iov_len;
			iov_[1].iov_base = (uint8_t*)iov_[1].iov_base + iov1_len;
			iov_[1].iov_len = iov_[1].iov_len - iov1_len;
			if (iov_[0].iov_len) {
				write_buff_.retrieveAll();
				iov_[0].iov_len = 0;
			}	
		}
		// 未发送完毕iov_[0]也即writebuff中数据，更新相关数据
		else {
			iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len;
			iov_[0].iov_len = iov_[0].iov_len - len;
			// 通过retrieve来移动read_pos
			write_buff_.retrieve(len);
		}
	} while (is_ET_);
	return len;
}

bool HttpConn::process() {

	// http_request_是维护了该连接的http请求对象
	http_request_.init();
	// 解析read_buff_中的数据并开始构造响应（因为数据是读到了read_buff中）（只解析了字段没有根据字段response）
	if (http_request_.parse(read_buff_)) {
		http_response_.init(src_dir_, http_request_.path(), http_request_.isKeepAlive(), 400);
	}
	// 这里将响应的一部分内容写到了write_buff_里（不包括响应文件的html代码，只有响应头）
	http_response_.makeResponse(write_buff_);

	// 让iov_[0]指向write_buff_，在write方法里面将iov_数据写到fd
	iov_[0].iov_base = write_buff_.beginReadPtr();
	iov_[0].iov_len = write_buff_.readableBytes();
	iov_count_ = 1;

	// 如果http请求还需要响应文件的话，让iov_[1]指向文件
	if (http_response_.fileLen > 0) {
		iov_[1].iov_base = http_response_.file();
		iov_[1].iov_len = http_response_.fileLen();
		iov_count_ = 2;
	}
	return true;

}

bool HttpConn::toWriteByte() {
	return iov_[0].iov_len + iov_[1].iov_len;
}

bool HttpConn::isKeepAlive() {
	return http_request_.isKeepAlive();
}