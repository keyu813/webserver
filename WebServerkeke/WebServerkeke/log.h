#ifndef LOG_H
#define LOG_H

#include "buffer.h"
#include "blockqueue.h"

// ����ģʽ
class Log {

public:
	static Log* Instance();
	// suffix�Ǻ�׺����˼
	// init��ʱ��ᴴ����Ӧ����־�ļ�
	void init(int level, const char* path = "./log", const char* suffix = ".log", int max_queue_capacity = 1024);

	// �첽д��־�Ĺ��з���
	static void flushLogThread();

	// ����д����־�ļ��ķ���������һ����formatд
	void write(int level, const char* format, ...);
	// ǿ��ˢ�»�����
	void flush();

	int getLevel();


private:
	Log() = default;
	// Log�������Ϊ���࣬��Ҫ����������
	virtual ~Log();

	const char* path_;
	const char* suffix_;
	// ��ʾ��ǰ��־ʵ���ļ���async���첽
	int level_;
	bool is_async_;

	Buffer buff_;
	// ��־����string����string��ʵ����BlockQueue
	std::unique_ptr<BlockQueue<std::string>> block_queue_ptr_;
	// ר�������첽д��־���̣߳�ִ��flushlogthread
	std::unique_ptr<std::thread> write_thread_;

	std::mutex mtx_;

	// ��־�ĵ�ǰ�������������
	int line_count_;

	// ��־�ļ�������࣬��¼�ڴ�����־ʵ������������һ��
	int now_day_;
	// ָ����ļ���ָ��
	FILE* fp_;

	// �첽д�뵽��־�ļ�������һ��д�̣߳��������߳̽�Ҫд������push�����к�д�߳�ȡ�����ݲ�д����־�ļ�
	void asyncWrite();

	static const int LOG_NAME_LEN = 256;
	static const int MAX_LINES_ = 50000;
	
};

// �����write�ǹ����̣߳��첽����½���־д��������
// log����ÿ��serverֻά��һ��
// log->getLevel���ص��Ǹ���־ʵ����level�����ⲿ�������־�ȼ�����level��0ʱ˵����ǰ����debugģʽ������д��ȫ���ȼ�����־
#define LOG_BASE(level, format, ...) \
	do {\
		Log* log = Log::Instance();\
		if (log->getLevel() <= level) {\
			log->write(level, format, ##__VA_ARGS__); \
			log->flush();\
		}\
	} while (0);


// ������־�ķ�����ͨ�����ĸ�����ⲿ����
// ##__VA_ARGS__��ʾ�ɱ�����ĺ꣬�滻���ǲ��������...����format�����Ǹ���
// ���� LOG_DEBUG("Tag:%d", tag)������ʹ�ã�����Tag:%d����format, tag����...
#define LOG_DEBUG(format, ...) do {LOG_BASE(0, format, ##__VA_ARGS__)} while(0);
#define LOG_INFO(format, ...) do {LOG_BASE(1, format, ##__VA_ARGS__)} while(0);
#define LOG_WARN(format, ...) do {LOG_BASE(2, format, ##__VA_ARGS__)} while(0);
#define LOG_ERROR(format, ...) do {LOG_BASE(3, format, ##__VA_ARGS__)} while(0);

#endif // !LOG_H
