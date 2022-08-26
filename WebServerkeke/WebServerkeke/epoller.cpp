#include "epoller.h"




// events_这个vector可以用max_event这个int来初始化
Epoller::Epoller(int max_event):epoll_fd_(epoll_create(512)), events_(max_event){
}

Epoller::~Epoller() {
	// close方法销毁掉对应的fd，定义在头文件中
	close(epoll_fd_);
	events_.clear();
}


int Epoller::wait(int timeout_ms) {
	// 数组第一个元素的地址即代表数组地址
	// vector.size返回的是size_type类型，因此强转
	return epoll_wait(epoll_fd_, &events_[0], static_cast<int>(events_.size()), timeout_ms);

}

// Epoller里面监听每一个fd都需要带上一个事件，只要对应的fd响应了相关事件wait才返回
bool Epoller::addFd(int fd, uint32_t ev) {
	epoll_event e = {0};
	e.events = ev;
	e.data.fd = fd;
	// epoll_ctl的作用是操作epoll_fd_所管理的事件
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