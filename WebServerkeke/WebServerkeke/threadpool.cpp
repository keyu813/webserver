#include "threadpool.h"


// ʵ����һ��ThreadPool��ʵ����һ��Pool(structҲ������make_shared����ʼ��)
explicit ThreadPool::ThreadPool(size_t threadCount = 8) : pool_(std::make_shared<Pool>()) {
	
	for (int i = 0; i < threadCount; i++) {
		// �����̣߳������൱�ڴ��������pool,���߳�ʹ��
		// thread()��������ݶ����߳���
		std::thread([pool = pool_] {
			// lock_guard�����Ϻ�mutex��࣬���Ƿ�װ���£������Զ������ͽ���
			// unique_lock�Ǹ���ģ�壬��mutex��Ϊģ���������mtx_��ʼ����Ӧ��������
			// unique_lock��lock_guard���ǹ�������RAII����װ�����Ĳ�������������lock_guardֻ���ڹ�������ʱ����������uniquelock����֮�⻹�ṩlock��unlock����
			// ͬlock_guard,unique_lock��ʵ��������ʱ�����
			// �̹߳����ĵ�һ�������Ի������ͨ���������������Ľӹ�Ȩ
			// �߳�ͬ�����ƣ������ź���
			std::unique_lock<std::mutex> locker(pool->mtx_);
			while (true) {
				if (!pool->tasks_.empty()) {
					// move�����˸�ֵ���죬ֱ��ת���ڴ�����Ȩ��������Ҫ���¹���һ��task��С���ڴ沢��ֵ��
					auto task = std::move(pool->tasks_.front());
					pool->tasks_.pop();
					// ���߳��õ���task���Ϳ����ͷ�����
					locker.unlock();
					task();
					// ΪʲôҪ����������
					locker.lock();
				}
				else if (pool->pool_closed_){
					break;
				}
				// ����Ϊ���Ҷ��в��رգ���ȴ���������
				else {
					// wait������������ǰ�̣߳������ͷ��̳߳��е���locker��������unlock()���̹߳��𣨽��뵱ǰ���������ĵȴ������У��������߳̿��Ծ�����
					// ��������ֻ�����unique_lock
					// wait��Ҫ��������ԭ���ǣ������ǰ�߳���Ϊ����Ϊ�������ˣ���ô���߳���Ҫ�ͷŶԶ��е���
					pool->cond_.wait(locker);
				}
			}
		
		}
		// detach��join�෴��detach��ʾÿ���̶߳��ǿɷ���ģ��߳�֮���໥����
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