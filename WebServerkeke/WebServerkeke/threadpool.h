#ifndef THREADPOOL_H
#define THREADPOOL_H


#include <mutex>
#include <condition_variable>
#include <queue>


class ThreadPool {

public:
	ThreadPool() = default;
	~ThreadPool();

	// 不加explicit的话，可能会出现ThreadPool temp = 10这样的写法
	// 上面的式子10会被创建一个ThreadPool对象，再赋给temp，不大合适
	// 加了explicit只允许使用ThreadPool temp(10)这种写法
	explicit ThreadPool(size_t threadCount = 8);

	// 移动构造，当类中的指针指向的堆空间较大时需要写这个
	ThreadPool(ThreadPool&&) = default;


	// 模板函数，实现写在头文件中
	template<class T>
	void addTask(T&& task) {
		{
			// 因为用的是lock_guard，所以在实例化locker的时候就会上锁，就会出现对mtx的竞争
			std::lock_guard<std::mutex> locker(pool_->mtx);
			// forward和move都是为了类型转换，其中move无条件转为右值，forward则根据<>中是左值还是右值，将()的类型转换为左值/右值
			// 这里是把task从右值转成了左值
			pool_->tasks_.emplace(std::forward<T>(task));
		}
		// notify_one因为不是线程池中的多线程干的事，是外部添加任务时导致的操作，因此也就不需要传入locker
		// 使用条件变量来通知工作线程有任务需要处理
		pool_->cond_.notify_one();
	}

private:
	struct Pool
	{
		// 其它线程往线程池中插任务时，以及工作线程从线程池中取任务时，都需要考虑锁
		std::mutex mtx_;
		// 如果队列为空时，工作线程需要wait，插入任务时再唤醒
		std::condition_variable cond_;
		bool pool_closed_;
		// task里面放的是function对象，即函数对象
		// 因为c++ stl是线程不安全的，多线程读/多线程写不同的容器是安全的，但一边读一边写就是不安全的，所以往往需要自己封装容器操作
		std::queue<std::function<void()>> tasks_;
	};
	// shared_ptr说明这个成员变量可以被多个不同对象访问
	// unique_ptr说明类这个成员变量只被对应对象管理，其它对象不能管理
	std::shared_ptr<Pool> pool_;

};



#endif // !THREADPOOL_H
