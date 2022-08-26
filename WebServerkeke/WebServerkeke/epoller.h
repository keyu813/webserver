#ifndef EPOLLER_H
#define EPOLLER_H

#include <vector>
// epoll.h exist only in linux
#include <sys/epoll.h>


class Epoller {

public:
	// ����дepoller��ʼ��Ҳ���Բ�������
	Epoller(int max_event = 1024);
	~Epoller();


	int wait(int timeout_ms);

	bool addFd(int fd, uint32_t ev);
	bool deleteFd(int fd);
	bool modifyFd(int fd, uint32_t ev);

	// Ϊ���õ�events_�е�fd���ѶԳ�Ա�����Ĳ�����װ�ɷ���
	// һ�����õ�fd��һ�����õ�fd�϶�Ӧ���¼�
	int getEventFd(int idx);
	uint32_t getEvent(int idx);

private:
	// ��ʶ�ں��е�epoll�¼���
	int epoll_fd_;
	// epoll_event��ͷ�ļ������룬��ʾ�ں�����Ҫ�������¼�
	std::vector<struct epoll_event> events_;
};


#endif // !EPOLLER_H
