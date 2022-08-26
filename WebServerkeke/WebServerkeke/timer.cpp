#include "timer.h"



Timer::Timer() {
	// ����64��С�Ŀռ�
	heap_timer_.reserve(64);
}

Timer::~Timer() {
	heap_timer_.clear();
}



// �������vector������±꣬�������±�i j����timenode
void Timer::swap(size_t i, size_t j) {
	std::swap(heap_timer_[i], heap_timer_[j]);
	fd_index_[heap_timer_[i].fd] = i;
	fd_index_[heap_timer_[j].fd] = j;
}

void Timer::siftUp(size_t index) {
	// �ҵ����ڵ�
	size_t i = (index - 1) / 2;
	while (i > 0) {
		// ������<���Ҹ��ڵ��С��������
		if (heap_timer_[i] < heap_timer_[index]) {
			break;
		}
		// ����heap��map�е�value
		swap(index, i);
		// �����±�ֵ
		index = i;
		i = (index - 1) / 2;
	}
}

// ���µ����ڵ��ʱ���ֹԽ���߽�n
void Timer::siftDown(size_t index, size_t n) {
	size_t n = heap_timer_.size();
	// j������
	size_t i = index;
	size_t j = index * 2 + 1;
	while (j < n) {
		// ���Һ����Ƿ�������ӣ���Ϊ���ڵ���Ҫ�ͺ����нϴ���Ǹ�����
		if (heap_timer_[j] < heap_timer_[j + 1] && j + 1 < end) {
			j++;
		}
		// �������
		if (heap_timer_[i] < heap_timer_[j]) {
			break;
		}
		swap(i, j);
		i = j;
		j = i * 2 + 1;
	}

}


void Timer::adjust(int fd, int time_out) {
	// ��fd��Ӧ�Ķ�ʱ����ʱ������time_out�����µ���
	heap_timer_[fd_index_[fd]].expire = Clock::now() + MS(time_out);
	siftDown(fd_index_[fd]);
}


// ����һ�ַ����Ǵ���TimerNode������һ���Ǵ���fd��ЩȻ���ٷ�װΪTimerNode
// ��Ҫֱ�Ӵ���TimerNode����Timer��صĶ���������������ⲿֻ�ṩfd��Щ����װ����Timer�ࣩ
bool Timer::addTimer(int fd, int time_out, const TimeoutCallBack& cb) {
	size_t index;
	if (fd_index_.find(fd) == fd_index_.end()) {
		index = heap_timer_.size();
		fd_index_[fd] = index;
		// {}ʵ����һ��struct����
		heap_timer_.push_back({fd, Clock::now() + MS(time_out), cb});
		siftUp(index);
	}
	else {
		// timernode�Ѿ����ڣ����²�����timernode
		std::map<int, size_t>::iterator it = fd_index_.find(fd);
		index = it->second;
		heap_timer_[index].expire = Clock::now() + MS(time_out);
		heap_timer_[index].cb = cb;
		siftDown(index);

	}
	
}


bool Timer::deleteTimer(int index) {
	
	size_t n = heap_timer_.size() - 1;
	// Ҫɾ���Ľڵ㻻����β�����µ�����
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
		// duration_cast�ڲ�ͬʱ�䵥λ֮�����ת��
		if (std::chrono::duration_cast<MS>(node.expire - Clock::now()).count() > 0) {
			// ��ǰ�Ѷ�Ԫ��û���ڣ�����
			break;
		}
		// node���ڣ���ִ�лص���������delete
		// function��������ú�������������ʹ��
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