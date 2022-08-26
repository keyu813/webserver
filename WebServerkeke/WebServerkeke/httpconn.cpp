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
	// ��ʼ��������
	read_buff_.retrieveAll();
	write_buff_.retrieveAll();
}

// read�ǽ����ݶ�������������
int HttpConn::read(int* error) {
	size_t len = -1;
	do {
		// ���ݶ���read_buff��
		len = read_buff_.readFd(fd_, error);
	} while (is_ET_);
	return len;
}


// write�ǽ����ݴ�д���壨���ܻ����ļ���д���������
// �����Ѿ��������Ӧ���ģ���Ҫ�ǽ��н���Ӧ����д��������˵��߼�
int HttpConn::write(int* error) {
	
	size_t len = -1;
	do {
		// writev���ϵͳ����һ��д�����������������˳��д��iov_[0],iov_[1]...�У����ص�lenӦ����iov_[0]/iov_[1]...���е�ֵ�ĺ�
		// writev��д��fd����д���������
		// iov_[0]����writebuff����makeresponse���Ѿ�������д��writebuff��Ҳ��iov_�ˣ�������д���������
		// iov_�����ݶ���process�����д������
		// reactorģʽ��writevϵͳ���ÿ��ܻ�����
		len = writev(fd_, iov_, iov_count_);
		// �������
		if (iov_[0].iov_len + iov_[1].iov_len == 0) {
			break;
		}
		// �Ѿ�������iov_[0]�е����ݣ�����iov_[1]�������writebuff
		else if (static_cast<size_t>(len) > iov_[0].iov_len) {
			uint8_t iov1_len = len - iov_[0].iov_len;
			iov_[1].iov_base = (uint8_t*)iov_[1].iov_base + iov1_len;
			iov_[1].iov_len = iov_[1].iov_len - iov1_len;
			if (iov_[0].iov_len) {
				write_buff_.retrieveAll();
				iov_[0].iov_len = 0;
			}	
		}
		// δ�������iov_[0]Ҳ��writebuff�����ݣ������������
		else {
			iov_[0].iov_base = (uint8_t*)iov_[0].iov_base + len;
			iov_[0].iov_len = iov_[0].iov_len - len;
			// ͨ��retrieve���ƶ�read_pos
			write_buff_.retrieve(len);
		}
	} while (is_ET_);
	return len;
}

bool HttpConn::process() {

	// http_request_��ά���˸����ӵ�http�������
	http_request_.init();
	// ����read_buff_�е����ݲ���ʼ������Ӧ����Ϊ�����Ƕ�����read_buff�У���ֻ�������ֶ�û�и����ֶ�response��
	if (http_request_.parse(read_buff_)) {
		http_response_.init(src_dir_, http_request_.path(), http_request_.isKeepAlive(), 400);
	}
	// ���ｫ��Ӧ��һ��������д����write_buff_���������Ӧ�ļ���html���룬ֻ����Ӧͷ��
	http_response_.makeResponse(write_buff_);

	// ��iov_[0]ָ��write_buff_����write�������潫iov_����д��fd
	iov_[0].iov_base = write_buff_.beginReadPtr();
	iov_[0].iov_len = write_buff_.readableBytes();
	iov_count_ = 1;

	// ���http������Ҫ��Ӧ�ļ��Ļ�����iov_[1]ָ���ļ�
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