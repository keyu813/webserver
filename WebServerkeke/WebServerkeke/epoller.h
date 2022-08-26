#ifndef EPOLLER_H
#define EPOLLER_H

#include <vector>
// epoll.h exist only in linux
#include <sys/epoll.h>


class Epoller {

public:
	// 这样写epoller初始化也可以不给参数
	Epoller(int max_event = 1024);
	~Epoller();


	int wait(int timeout_ms);

	bool addFd(int fd, uint32_t ev);
	bool deleteFd(int fd);
	bool modifyFd(int fd, uint32_t ev);

	// 为了拿到events_中的fd，把对成员变量的操作封装成方法
	// 一个是拿到fd，一个是拿到fd上对应的事件
	int getEventFd(int idx);
	uint32_t getEvent(int idx);

private:
	// 标识内核中的epoll事件表
	int epoll_fd_;
	// epoll_event在头文件中引入，表示内核所需要监听的事件
	std::vector<struct epoll_event> events_;
};


#endif // !EPOLLER_H
