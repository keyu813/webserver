#include "log.h"

#include <stdarg.h>


Log* Log::Instance() {
	static Log log;
	return &log;
}

// 只有构造函数可以列表初始化
void Log::init(int level, const char* path, const char* suffix, int max_queue_capacity) {
	
	level_ = level;
	path_ = path;
	suffix_ = suffix;

	if (max_queue_capacity > 0) {
		is_async_ = true;
		if (!block_queue_ptr_) {
			// 这里为什么要移动语义？节省开销，因为如果直接赋值的话会调用赋值构造，temp_deque指向的是blockqueue，其内部维护了比较大的内存空间（deque），如果不用移动语义那么block_queue_ptr也需要开辟这么一块空间再指向它
			// move转移unique_ptr指向的内存所有权
			std::unique_ptr<BlockQueue<std::string>> temp_deque(new BlockQueue<std::string>);
			block_queue_ptr_ = std::move(temp_deque);
			// flushlogthread会调用asynwrite，里面有一个while循环，因此该线程会不断检查阻塞队列来写日志
			std::unique_ptr<std::thread> temp_thread(new std::thread(flushLogThread));
			write_thread_ = std::move(temp_thread);
		}
	}
	else {
		is_async_ = false;
	}

	line_count_ = 0;
	char file_name[LOG_NAME_LEN] = { 0 };

	// 拿到当前的时间
	time_t timer = time(nullptr);
	struct tm *sys_time = localtime(&timer);
	struct tm t = *sys_time;
	
	// snprintf的作用是将path_,tm_year,tm_mon,tm_mday,suffix这些参数（可变参数）以%s/%04d...的方式拷贝到filename中
	snprintf(file_name, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s", path_, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_);
	now_day_ = t.tm_mday;

	{
		std::lock_guard<std::mutex> locker(mtx_);
		buff_.retrieveAll();
		// webserver初始化的时候会实例化一个日志系统并创建日志文件file_name
		fp_ = fopen(file_name, "a");
	
	}

}

// main运行完后会对单例生成的对象析构掉
Log::~Log() {
	// 写线程还在运行，将队列中剩余的元素写到文件中
	if (write_thread_->joinable) {
		while (!block_queue_ptr_->empty()) {
			block_queue_ptr_->flush();
		}
		// 写完后关闭队列和写线程
		block_queue_ptr_->close();
		write_thread_->join();
	}
	if (fp_) {
		std::lock_guard<std::mutex> locker(mtx_);
		flush();
		fclose(fp_);
	}

}

// ...表示可变参数，即可以没有参数也可以一个或多个参数
void Log::write(int level, const char* format, ...) {
	struct timeval now = { 0, 0 };
	gettimeofday(&now, nullptr);
	time_t t_sec = now.tv_sec;
	struct tm* sys_time = localtime(&t_sec);
	struct tm t = *sys_time;
	va_list va_list;

	// 如果当前对象维护的天数和当前天数不同或者日志数超过最大日志数，需要创建新的日志文件
	if (now_day_ != t.tm_mday || line_count_ % MAX_LINES_ == 0) {
		std::unique_lock<std::mutex> locker(mtx_);
		locker.unlock();

		// 更改新的tail
		char tail[36] = { 0 };
		snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

		// 更改新的日志文件的名字
		char new_file[LOG_NAME_LEN];
		if (now_day_ != t.tm_mday) {
			// 如果是因为时间对不上，创建新log,更新创建时间和行数
			snprintf(new_file, LOG_NAME_LEN - 72, "%s/%s%s", path_, tail, suffix_);
			now_day_ = t.tm_mday;
			line_count_ = 0;
		}
		else {
			// 单个日志文件行数超过最大行数，比如最大为20行，若当前30行，就要拆分两个文件
			snprintf(new_file, LOG_NAME_LEN - 72, "%s/%s-%d%s", path_, tail, (line_count_ / MAX_LINES_), suffix_);
		}

		locker.lock();
		flush();
		fp_ = fopen(new_file, "a");


	}
	// 不需要创建新文件，在当前日志文件中写
	{
		std::unique_lock<std::mutex> locker(mtx_);
		line_count_++;
		// 确定要写的内容，再根据是否异步决定如何写入到文件中
		// 日志当中添加时间，是写到了buff_中，再放到队列里面
		int n = snprintf(buff_.beginWritePtrNoConst(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
		// 移动write_pos_
		buff_.hasWritten(n);
		// 添加日志头
		switch (level)
		{
		case 0:
			buff_.append("[debug]: ", 9);
			break;
		case 1:
			buff_.append("[info]: ", 9);
			break;
		case 2:
			buff_.append("[warn]: ", 9);
			break;
		case 3:
			buff_.append("[error]: ", 9);
			break;
		default:
			break;
		}
		// 将传入的format参数赋值给va_list
		va_start(va_list, format);
		// va_list是一组可变参数，用va_list来替换掉format中的一些格式化字符串，再写到buff_中
		int m = vsnprintf(buff_.beginWritePtrNoConst(), buff_.writeableBytes(), format, va_list);
		va_end(va_list);

		buff_.hasWritten(m);
		buff_.append("\n\0", 2);
		// 到这里buff_中已经存储了所有日志内容
		if (is_async_ && !block_queue_ptr_->full()) {
			// 异步，将buff_中的内容放到队列中，交给异步写线程去写到文件中
			std::string str(buff_.beginReadPtr(), buff_.readableBytes());
			buff_.retrieveAll();
			block_queue_ptr_->push_back(str);
		}
		else {
			// 同步，直接在这里将日志写到文件中，可能阻塞
			fputs(buff_.beginReadPtr(), fp_);
		}
	
	}
}

// 给写线程调用的方法
void Log::flushLogThread() {
	Log::Instance()->asyncWrite();
}

// 异步写，从队列中取出具体的日志内容写入到文件中
// 是写线程干的事
void Log::asyncWrite() {
	std::string str = "";
	while (block_queue_ptr_->pop(str)) {
		std::lock_guard<std::mutex> locker(mtx_);
		// fputs将str写到fp_中，将日志写到文件中最后是用fputs
		fputs(str.c_str(), fp_);
	}

}


void Log::flush() {
	
	if (is_async_) {
		block_queue_ptr_->flush();
	}
	// 强迫将缓冲区内数据写到fp_指定文件中
	fflush(fp_);

}


int Log::getLevel() {
	std::lock_guard<std::mutex> locker(mtx_);
	return level_;
}

