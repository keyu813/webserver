#include "threadpool.h"


// 实例化一个ThreadPool先实例化一个Pool(struct也可以用make_shared来初始化)
explicit ThreadPool::ThreadPool(size_t threadCount = 8) : pool_(std::make_shared<Pool>()) {
	
	for (int i = 0; i < threadCount; i++) {
		// 创建线程，这里相当于传入个参数pool,给线程使用
		// thread()里面的内容都是线程体
		std::thread([pool = pool_] {
			// lock_guard功能上和mutex差不多，就是封装了下，可以自动上锁和解锁
			// unique_lock是个类模板，用mutex作为模板参数，用mtx_初始化对应的锁对象
			// unique_lock和lock_guard都是管理锁，RAII风格封装对锁的操作，区别在于lock_guard只能在构造析构时加锁放锁，uniquelock除此之外还提供lock和unlock方法
			// 同lock_guard,unique_lock在实例化对象时获得锁
			// 线程工作的第一步：尝试获得锁，通过竞争来获得任务的接管权
			// 线程同步机制：锁、信号量
			std::unique_lock<std::mutex> locker(pool->mtx_);
			while (true) {
				if (!pool->tasks_.empty()) {
					// move避免了赋值构造，直接转移内存所有权（否则又要重新构造一个task大小的内存并赋值）
					auto task = std::move(pool->tasks_.front());
					pool->tasks_.pop();
					// 当线程拿到了task，就可以释放锁了
					locker.unlock();
					task();
					// 为什么要再锁起来？
					locker.lock();
				}
				else if (pool->pool_closed_){
					break;
				}
				// 队列为空且队列不关闭，则等待放入任务
				else {
					// wait函数会阻塞当前线程，并且释放线程持有的锁locker，即调用unlock()，线程挂起（进入当前条件变量的等待队列中），其它线程可以竞争锁
					// 条件变量只能配合unique_lock
					// wait需要传入锁的原因是：如果当前线程因为队列为空阻塞了，那么该线程需要释放对队列的锁
					pool->cond_.wait(locker);
				}
			}
		
		}
		// detach与join相反，detach表示每个线程都是可分离的，线程之间相互独立
		).detach();
	}

}

ThreadPool::~ThreadPool() {
	if (static_cast<bool>(pool_)) {
		{
			std::lock_guard<std::mutex> locker(pool_->mtx_);
			pool_->pool_closed_ = true;
		}
		pool_->cond_.notify_all();
	}

}