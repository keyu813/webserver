#ifndef BLOCKQUEUE_H
#define BLOCKQUEUE_H


#include <deque>
#include <mutex>
#include <condition_variable>



template<class T>
// 阻塞队列封装了个生产者消费者模型，阻塞队列作为生产者和消费者共享的缓冲区
// 为了实现线程间数据同步，生产者和消费者线程共享一个缓冲区
class BlockQueue {
public:
	// 类如果没有默认构造，只有在某些特定情况下（比如虚函数），编译器才会生成对应的默认构造
	// 没给默认构造，不能用默认初始化的方式来初始化它
	explicit BlockQueue(size_t capacity = 1000);
	~BlockQueue();


	// 包装了stl中队列的一些方法
	T front();
	T back();
	size_t size();
	size_t capacity();
	void push_back(const T& item);
	void push_front(const T& item);
	bool full();
	bool empty();
	bool clear();
	bool pop(T& item);
	void flush();
	void close();

private:
	// 阻塞队列内部用双端队列实现，且对队列的访问必须加锁
	std::deque<T> dq_;
	size_t capacity_;
	std::mutex mtx_;
	// push的成员是生产者，pop的是消费者
	// 条件变量，用于线程间通信
	// 对producer来说，当队列满的时候push失败，该线程会wait，当pop成功的时候会notify producer等待队列中的第一个线程来生产数据
	// 对consumer来说，当队列空的时候pop失败，线程会wait，当push成功的时候会notify consumer对应等待队列中的第一个线程来消费数据
	std::condition_variable cv_consumer_;
	std::condition_variable cv_producer_;
	

};

// 往队列里放类对象，所以模板是class
template<class T>
BlockQueue<T>::BlockQueue(size_t capacity) : capacity_(capacity) {

}

template<class T>
BlockQueue<T>::~BlockQueue() {
	{
		// 拿到mtx_这个锁，赋给locker
		// lock_guard/unique_lock在构造的时候就要求要拿到相应的锁
		std::lock_guard<std::mutex> locker(mtx_);
		dq_.clear();
	}
	// 唤醒所有等待的线程
	cv_consumer_.notify_all();
	cv_producer_.notify_all();
}

template<class T>
T BlockQueue<T>::front() {
	std::lock_guard<std::mutex> locker(mtx_);
	return dq_.front();
}

template<class T>
T BlockQueue<T>::back() {
	std::lock_guard<std::mutex> locker(mtx_);
	return dq_.back();
}

template<class T>
size_t BlockQueue<T>::size() {
	std::lock_guard<std::mutex> locker(mtx_);
	return dq_.size();
}

template<class T>
size_t BlockQueue<T>::capacity() {
	std::lock_guard<std::mutex> locker(mtx_);
	return capacity_;
}

// 如果有多个线程同时wait某个资源，那么当出现了这个资源后，多线程就会有一种竞争关系
template<class T>
void BlockQueue<T>::push_back(const T& item) {
	std::unique_lock<std::mutex> locker(mtx_);
	// 队列长度最多为capacity，否则就要唤醒前面的
	while (dq_.size() > capacity_) {
		// wait会阻塞当前线程，直到被唤醒
		// cv_producer_这个条件变量会维护一个自己的队列，wait函数会将当前线程放到这个队列中
		// wait函数内部会释放掉locker，这样其它线程就能获得锁且当前线程会加入到队列中。（这就是为什么要传locker作为参数的理由）
		// 这里要用while，因为可能存在虚假唤醒的问题
		cv_producer_.wait(locker);
	}
	dq_.push_back(item);
	cv_consumer_.notify_one();
}

template<class T>
void BlockQueue<T>::push_front(const T& item) {
	std::unique_lock<std::mutex> locker(mtx_);
	while (dq_.size() > capacity_) {
		cv_producer_.wait(locker);
	}
	dq_.push_front(item);
	cv_consumer_.notify_one();
}

template<class T>
bool BlockQueue<T>::full() {
	std::lock_guard<std::mutex> locker(mtx_);
	return dq_.size() >= capacity_;
}

template<class T>
bool BlockQueue<T>::empty() {
	std::lock_guard<std::mutex> locker(mtx_);
	return dq_.empty();
}

template<class T>
bool BlockQueue<T>::clear() {
	std::lock_guard<std::mutex> locker(mtx_);
	return dq_.clear();
}

template<class T>
bool BlockQueue<T>::pop(T& item) {
	std::unique_lock<std::mutex> locker(mtx_);
	while (dq_.empty()) {
		cv_consumer_.wait(locker);
	}
	item = dq_.front();
	dq_.pop_front();
	cv_producer_.notify_one();
	return true;
}

template<class T>
void BlockQueue<T>::flush() {
	// 唤醒在cv_consumer_上等待的某个线程，来从队列中取走元素
	cv_consumer_.notify_one();
};

template<class T>
void BlockQueue<T>::close() {
	{
		std::lock_guard<std::mutex> locker(mtx_);
		dq_.clear();
	}
	cv_producer_.notify_all();
	cv_consumer_.notify_all();
};






#endif // !BLOCKQUEUE_H
