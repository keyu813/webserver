#ifndef BLOCKQUEUE_H
#define BLOCKQUEUE_H


#include <deque>
#include <mutex>
#include <condition_variable>



template<class T>
// �������з�װ�˸�������������ģ�ͣ�����������Ϊ�����ߺ������߹���Ļ�����
// Ϊ��ʵ���̼߳�����ͬ���������ߺ��������̹߳���һ��������
class BlockQueue {
public:
	// �����û��Ĭ�Ϲ��죬ֻ����ĳЩ�ض�����£������麯�������������Ż����ɶ�Ӧ��Ĭ�Ϲ���
	// û��Ĭ�Ϲ��죬������Ĭ�ϳ�ʼ���ķ�ʽ����ʼ����
	explicit BlockQueue(size_t capacity = 1000);
	~BlockQueue();


	// ��װ��stl�ж��е�һЩ����
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
	// ���������ڲ���˫�˶���ʵ�֣��ҶԶ��еķ��ʱ������
	std::deque<T> dq_;
	size_t capacity_;
	std::mutex mtx_;
	// push�ĳ�Ա�������ߣ�pop����������
	// ���������������̼߳�ͨ��
	// ��producer��˵������������ʱ��pushʧ�ܣ����̻߳�wait����pop�ɹ���ʱ���notify producer�ȴ������еĵ�һ���߳�����������
	// ��consumer��˵�������пյ�ʱ��popʧ�ܣ��̻߳�wait����push�ɹ���ʱ���notify consumer��Ӧ�ȴ������еĵ�һ���߳�����������
	std::condition_variable cv_consumer_;
	std::condition_variable cv_producer_;
	

};

// ������������������ģ����class
template<class T>
BlockQueue<T>::BlockQueue(size_t capacity) : capacity_(capacity) {

}

template<class T>
BlockQueue<T>::~BlockQueue() {
	{
		// �õ�mtx_�����������locker
		// lock_guard/unique_lock�ڹ����ʱ���Ҫ��Ҫ�õ���Ӧ����
		std::lock_guard<std::mutex> locker(mtx_);
		dq_.clear();
	}
	// �������еȴ����߳�
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

// ����ж���߳�ͬʱwaitĳ����Դ����ô�������������Դ�󣬶��߳̾ͻ���һ�־�����ϵ
template<class T>
void BlockQueue<T>::push_back(const T& item) {
	std::unique_lock<std::mutex> locker(mtx_);
	// ���г������Ϊcapacity�������Ҫ����ǰ���
	while (dq_.size() > capacity_) {
		// wait��������ǰ�̣߳�ֱ��������
		// cv_producer_�������������ά��һ���Լ��Ķ��У�wait�����Ὣ��ǰ�̷߳ŵ����������
		// wait�����ڲ����ͷŵ�locker�����������߳̾��ܻ�����ҵ�ǰ�̻߳���뵽�����С��������ΪʲôҪ��locker��Ϊ���������ɣ�
		// ����Ҫ��while����Ϊ���ܴ�����ٻ��ѵ�����
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
	// ������cv_consumer_�ϵȴ���ĳ���̣߳����Ӷ�����ȡ��Ԫ��
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
