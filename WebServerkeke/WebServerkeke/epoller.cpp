#include "epoller.h"




// events_���vector������max_event���int����ʼ��
Epoller::Epoller(int max_event):epoll_fd_(epoll_create(512)), events_(max_event){
}

Epoller::~Epoller() {
	// close�������ٵ���Ӧ��fd��������ͷ�ļ���
	close(epoll_fd_);
	events_.clear();
}


int Epoller::wait(int timeout_ms) {
	// �����һ��Ԫ�صĵ�ַ�����������ַ
	// vector.size���ص���size_type���ͣ����ǿת
	return epoll_wait(epoll_fd_, &events_[0], static_cast<int>(events_.size()), timeout_ms);

}

// Epoller�������ÿһ��fd����Ҫ����һ���¼���ֻҪ��Ӧ��fd��Ӧ������¼�wait�ŷ���
bool Epoller::addFd(int fd, uint32_t ev) {
	epoll_event e = {0};
	e.events = ev;
	e.data.fd = fd;
	// epoll_ctl�������ǲ���epoll_fd_��������¼�
	return 0 == epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &e);
	
}

bool Epoller::deleteFd(int fd) {
	epoll_event e;
	return 0 == epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, &e);
}

bool Epoller::modifyFd(int fd, uint32_t ev) {
	epoll_event e;
	e.events = ev;
	e.data.fd = fd;
	return 0 == epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd, &e);
}

int Epoller::getEventFd(int idx) {
	return events_[idx].data.fd;
}

uint32_t Epoller::getEvent(int idx) {
	return events_[idx].events;
}