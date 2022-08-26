#ifndef LOG_H
#define LOG_H

#include "buffer.h"
#include "blockqueue.h"

// 单例模式
class Log {

public:
	static Log* Instance();
	// suffix是后缀的意思
	// init的时候会创建对应的日志文件
	void init(int level, const char* path = "./log", const char* suffix = ".log", int max_queue_capacity = 1024);

	// 异步写日志的公有方法
	static void flushLogThread();

	// 具体写入日志文件的方法，按照一定的format写
	void write(int level, const char* format, ...);
	// 强制刷新缓冲区
	void flush();

	int getLevel();


private:
	Log() = default;
	// Log类可能作为父类，需要虚析构函数
	virtual ~Log();

	const char* path_;
	const char* suffix_;
	// 表示当前日志实例的级别，async是异步
	int level_;
	bool is_async_;

	Buffer buff_;
	// 日志都是string，用string来实例化BlockQueue
	std::unique_ptr<BlockQueue<std::string>> block_queue_ptr_;
	// 专门用来异步写日志的线程，执行flushlogthread
	std::unique_ptr<std::thread> write_thread_;

	std::mutex mtx_;

	// 日志的当前行数和最大行数
	int line_count_;

	// 日志文件按天分类，记录在创建日志实例的那天是哪一天
	int now_day_;
	// 指向打开文件的指针
	FILE* fp_;

	// 异步写入到日志文件，创建一个写线程，当工作线程将要写的内容push进队列后，写线程取出内容并写入日志文件
	void asyncWrite();

	static const int LOG_NAME_LEN = 256;
	static const int MAX_LINES_ = 50000;
	
};

// 这里的write是工作线程，异步情况下将日志写到队列中
// log对象每个server只维护一个
// log->getLevel返回的是该日志实例的level，是外部定义的日志等级。当level在0时说明当前开了debug模式，可以写入全部等级的日志
#define LOG_BASE(level, format, ...) \
	do {\
		Log* log = Log::Instance();\
		if (log->getLevel() <= level) {\
			log->write(level, format, ##__VA_ARGS__); \
			log->flush();\
		}\
	} while (0);


// 所有日志的方法都通过这四个宏给外部调用
// ##__VA_ARGS__表示可变参数的宏，替换的是参数里面的...（即format后面那个）
// 形如 LOG_DEBUG("Tag:%d", tag)这样来使用，其中Tag:%d属于format, tag属于...
#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);

#endif // !LOG_H
