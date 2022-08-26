#include "timer.h"



Timer::Timer() {
	// 申请64大小的空间
	heap_timer_.reserve(64);
}

Timer::~Timer() {
	heap_timer_.clear();
}



// 传入的是vector数组的下标，即交换下标i j处的timenode
void Timer::swap(size_t i, size_t j) {
	std::swap(heap_timer_[i], heap_timer_[j]);
	fd_index_[heap_timer_[i].fd] = i;
	fd_index_[heap_timer_[j].fd] = j;
}

void Timer::siftUp(size_t index) {
	// 找到父节点
	size_t i = (index - 1) / 2;
	while (i > 0) {
		// 重载了<，且父节点更小，不调整
		if (heap_timer_[i] < heap_timer_[index]) {
			break;
		}
		// 交换heap和map中的value
		swap(index, i);
		// 调整下标值
		index = i;
		i = (index - 1) / 2;
	}
}

// 向下调整节点的时候防止越过边界n
void Timer::siftDown(size_t index, size_t n) {
	size_t n = heap_timer_.size();
	// j是左孩子
	size_t i = index;
	size_t j = index * 2 + 1;
	while (j < n) {
		// 看右孩子是否大于左孩子，因为父节点是要和孩子中较大的那个交换
		if (heap_timer_[j] < heap_timer_[j + 1] && j + 1 < end) {
			j++;
		}
		// 调整完毕
		if (heap_timer_[i] < heap_timer_[j]) {
			break;
		}
		swap(i, j);
		i = j;
		j = i * 2 + 1;
	}

}


void Timer::adjust(int fd, int time_out) {
	// 将fd对应的定时器的时间增加time_out并向下调整
	heap_timer_[fd_index_[fd]].expire = Clock::now() + MS(time_out);
	siftDown(fd_index_[fd]);
}


// 这里一种方法是传入TimerNode，另外一种是传入fd这些然后再封装为TimerNode
// 不要直接传入TimerNode，跟Timer相关的都交给相关类做（外部只提供fd这些，封装交给Timer类）
bool Timer::addTimer(int fd, int time_out, const TimeoutCallBack& cb) {
	size_t index;
	if (fd_index_.find(fd) == fd_index_.end()) {
		index = heap_timer_.size();
		fd_index_[fd] = index;
		// {}实例化一个struct对象
		heap_timer_.push_back({fd, Clock::now() + MS(time_out), cb});
		siftUp(index);
	}
	else {
		// timernode已经存在，更新并调整timernode
		std::map<int, size_t>::iterator it = fd_index_.find(fd);
		index = it->second;
		heap_timer_[index].expire = Clock::now() + MS(time_out);
		heap_timer_[index].cb = cb;
		siftDown(index);

	}
	
}


bool Timer::deleteTimer(int index) {
	
	size_t n = heap_timer_.size() - 1;
	// 要删除的节点换到队尾，向下调整堆
	if (index < n) {
		swap(index, n);
		siftDown(index, n);
	}
	fd_index_.erase(heap_timer_.back().fd);
	heap_timer_.pop_back();
}

void Timer::tick() {

	while (!heap_timer_.empty()) {
		TimeNode node = heap_timer_.front();
		// duration_cast在不同时间单位之间进行转换
		if (std::chrono::duration_cast<MS>(node.expire - Clock::now()).count() > 0) {
			// 当前堆顶元素没过期，跳出
			break;
		}
		// node过期，先执行回调函数，再delete
		// function对象可以让函数当作属性来使用
		node.cb();
		deleteTimer(0);
	}
}


int Timer::getNextTick() {

	tick();
	size_t res = std::chrono::duration_cast<MS>(heap_timer_.front().expire - Clock::now()).count();
	if (res < 0) {
		res = 0;
	}
	return res;
	


}