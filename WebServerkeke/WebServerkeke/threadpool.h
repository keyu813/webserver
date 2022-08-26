#ifndef THREADPOOL_H
#define THREADPOOL_H


#include <mutex>
#include <condition_variable>
#include <queue>


class ThreadPool {

public:
	ThreadPool() = default;
	~ThreadPool();

	// ����explicit�Ļ������ܻ����ThreadPool temp = 10������д��
	// �����ʽ��10�ᱻ����һ��ThreadPool�����ٸ���temp���������
	// ����explicitֻ����ʹ��ThreadPool temp(10)����д��
	explicit ThreadPool(size_t threadCount = 8);

	// �ƶ����죬�����е�ָ��ָ��Ķѿռ�ϴ�ʱ��Ҫд���
	ThreadPool(ThreadPool&&) = default;


	// ģ�庯����ʵ��д��ͷ�ļ���
	template<class T>
	void addTask(T&& task) {
		{
			// ��Ϊ�õ���lock_guard��������ʵ����locker��ʱ��ͻ��������ͻ���ֶ�mtx�ľ���
			std::lock_guard<std::mutex> locker(pool_->mtx);
			// forward��move����Ϊ������ת��������move������תΪ��ֵ��forward�����<>������ֵ������ֵ����()������ת��Ϊ��ֵ/��ֵ
			// �����ǰ�task����ֵת������ֵ
			pool_->tasks_.emplace(std::forward<T>(task));
		}
		// notify_one��Ϊ�����̳߳��еĶ��̸߳ɵ��£����ⲿ�������ʱ���µĲ��������Ҳ�Ͳ���Ҫ����locker
		// ʹ������������֪ͨ�����߳���������Ҫ����
		pool_->cond_.notify_one();
	}

private:
	struct Pool
	{
		// �����߳����̳߳��в�����ʱ���Լ������̴߳��̳߳���ȡ����ʱ������Ҫ������
		std::mutex mtx_;
		// �������Ϊ��ʱ�������߳���Ҫwait����������ʱ�ٻ���
		std::condition_variable cond_;
		bool pool_closed_;
		// task����ŵ���function���󣬼���������
		// ��Ϊc++ stl���̲߳���ȫ�ģ����̶߳�/���߳�д��ͬ�������ǰ�ȫ�ģ���һ�߶�һ��д���ǲ���ȫ�ģ�����������Ҫ�Լ���װ��������
		std::queue<std::function<void()>> tasks_;
	};
	// shared_ptr˵�������Ա�������Ա������ͬ�������
	// unique_ptr˵���������Ա����ֻ����Ӧ����������������ܹ���
	std::shared_ptr<Pool> pool_;

};



#endif // !THREADPOOL_H
