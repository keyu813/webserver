// ≥¢ ‘ ÷–¥œﬂ≥Ã≥ÿ


#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <memory>
#include <queue>
#include <thread>
#include <mutex>
#include <condtion_variable>



class ThreadPool {

public:

	ThreadPool() = default;
	~ThreadPool();

	explicit ThreadPool(int nums) {
		
	}

	ThreadPool(ThreadPool&&) = default;

	



private:
	
	struct Pool {
		std::queue<std::function<void()>> tasks_;
		std::mutex mtx_;
		std::condition_variable cv_;
	};

	std::shared_ptr<Pool> pool_;


}