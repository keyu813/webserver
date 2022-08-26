#ifndef TIMER_H
#define TIMER_H


#include <functional>
#include <chrono>
#include <vector>
#include <map>

// ��http���Ӷ�Ӧ��timer����ʱִ�еĺ���
// function�������Ϊc++�еĺ���ָ�룬����function����TimeoutCallBack������ֵvoid�����������
typedef std::function<void()> TimeoutCallBack;

// chrono��ʱ�ӿ�
// �õ�һ��Clock�࣬ʱ�䵥λΪTimeStamp
typedef std::chrono::high_resolution_clock Clock;
// ��Ҫ������λת������time��λת��Ϊms
typedef std::chrono::milliseconds MS;
// �õ�Clock��ʱ�䵥λTimeStamp
typedef Clock::time_point TimeStamp;

// һ��http���ӻ�����һ��TimeNode
struct TimeNode
{
	int fd;
	TimeStamp expire;
	TimeoutCallBack cb;
	// ����<�����ڱȽ�����TimeNode�Ĵ�С
	bool operator<(const TimeNode& t) {
		return expire < t.expire;
	}

};

class Timer {

public:

	Timer();
	~Timer();
	bool addTimer(int fd, int time_out, const TimeoutCallBack& cb);
	// vectorʵ�֣�����ɾ������index����Ԫ�ز����µ�����
	bool deleteTimer(int index);
	
	// ���һ�ζѣ����ѹ��ڵĽڵ�ɾ����
	void tick();
	// ����һ��int��������int�������С�ļ�ʱ��������
	int getNextTick();



	void swap(size_t i, size_t j);
	// ��fd_index_�е�index��timer���ϻ����µ�������Ϊ�ѵ�ʵ����vector
	void siftUp(size_t index);
	void siftDown(size_t index);

	// ��fd��Ӧ�Ķ�ʱ������ʱ���޸�Ϊtime_out
	void adjust(int fd, int time_out);


private:
	// vectorʵ��С��heap
	// ����һ����ȫ����������������м䲻���п�ֵ
	std::vector<TimeNode> heap_timer_;
	// key�����ӵ�fd,value����Ӧ��fd��vector�е��±�
	std::map<int, size_t> fd_index_;


};


#endif // !TIMER_H
