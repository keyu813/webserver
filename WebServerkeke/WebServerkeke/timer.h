#ifndef TIMER_H
#define TIMER_H


#include <functional>
#include <chrono>
#include <vector>
#include <map>

// 当http连接对应的timer过期时执行的函数
// function可以理解为c++中的函数指针，定义function对象TimeoutCallBack，返回值void，不传入参数
typedef std::function<void()> TimeoutCallBack;

// chrono：时钟库
// 拿到一个Clock类，时间单位为TimeStamp
typedef std::chrono::high_resolution_clock Clock;
// 主要用作单位转换，将time单位转换为ms
typedef std::chrono::milliseconds MS;
// 拿到Clock的时间单位TimeStamp
typedef Clock::time_point TimeStamp;

// 一个http连接会生成一个TimeNode
struct TimeNode
{
	int fd;
	TimeStamp expire;
	TimeoutCallBack cb;
	// 重载<，便于比较两个TimeNode的大小
	bool operator<(const TimeNode& t) {
		return expire < t.expire;
	}

};

class Timer {

public:

	Timer();
	~Timer();
	bool addTimer(int fd, int time_out, const TimeoutCallBack& cb);
	// vector实现，允许删除堆中index处的元素并重新调整堆
	bool deleteTimer(int index);
	
	// 清除一次堆，将已过期的节点删除掉
	void tick();
	// 返回一个int，经过该int后堆中最小的计时器将过期
	int getNextTick();



	void swap(size_t i, size_t j);
	// 将fd_index_中第index的timer向上或向下调整，因为堆的实现用vector
	void siftUp(size_t index);
	void siftDown(size_t index);

	// 将fd对应的定时器过期时间修改为time_out
	void adjust(int fd, int time_out);


private:
	// vector实现小根heap
	// 堆是一颗完全二叉树，因此数组中间不会有空值
	std::vector<TimeNode> heap_timer_;
	// key是连接的fd,value是相应的fd在vector中的下标
	std::map<int, size_t> fd_index_;


};


#endif // !TIMER_H
