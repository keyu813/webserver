#include "log.h"

#include <stdarg.h>


Log* Log::Instance() {
	static Log log;
	return &log;
}

// ֻ�й��캯�������б��ʼ��
void Log::init(int level, const char* path, const char* suffix, int max_queue_capacity) {
	
	level_ = level;
	path_ = path;
	suffix_ = suffix;

	if (max_queue_capacity > 0) {
		is_async_ = true;
		if (!block_queue_ptr_) {
			// ����ΪʲôҪ�ƶ����壿��ʡ��������Ϊ���ֱ�Ӹ�ֵ�Ļ�����ø�ֵ���죬temp_dequeָ�����blockqueue�����ڲ�ά���˱Ƚϴ���ڴ�ռ䣨deque������������ƶ�������ôblock_queue_ptrҲ��Ҫ������ôһ��ռ���ָ����
			// moveת��unique_ptrָ����ڴ�����Ȩ
			std::unique_ptr<BlockQueue<std::string>> temp_deque(new BlockQueue<std::string>);
			block_queue_ptr_ = std::move(temp_deque);
			// flushlogthread�����asynwrite��������һ��whileѭ������˸��̻߳᲻�ϼ������������д��־
			std::unique_ptr<std::thread> temp_thread(new std::thread(flushLogThread));
			write_thread_ = std::move(temp_thread);
		}
	}
	else {
		is_async_ = false;
	}

	line_count_ = 0;
	char file_name[LOG_NAME_LEN] = { 0 };

	// �õ���ǰ��ʱ��
	time_t timer = time(nullptr);
	struct tm *sys_time = localtime(&timer);
	struct tm t = *sys_time;
	
	// snprintf�������ǽ�path_,tm_year,tm_mon,tm_mday,suffix��Щ�������ɱ��������%s/%04d...�ķ�ʽ������filename��
	snprintf(file_name, LOG_NAME_LEN - 1, "%s/%04d_%02d_%02d%s", path_, t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, suffix_);
	now_day_ = t.tm_mday;

	{
		std::lock_guard<std::mutex> locker(mtx_);
		buff_.retrieveAll();
		// webserver��ʼ����ʱ���ʵ����һ����־ϵͳ��������־�ļ�file_name
		fp_ = fopen(file_name, "a");
	
	}

}

// main��������Ե������ɵĶ���������
Log::~Log() {
	// д�̻߳������У���������ʣ���Ԫ��д���ļ���
	if (write_thread_->joinable) {
		while (!block_queue_ptr_->empty()) {
			block_queue_ptr_->flush();
		}
		// д���رն��к�д�߳�
		block_queue_ptr_->close();
		write_thread_->join();
	}
	if (fp_) {
		std::lock_guard<std::mutex> locker(mtx_);
		flush();
		fclose(fp_);
	}

}

// ...��ʾ�ɱ������������û�в���Ҳ����һ����������
void Log::write(int level, const char* format, ...) {
	struct timeval now = { 0, 0 };
	gettimeofday(&now, nullptr);
	time_t t_sec = now.tv_sec;
	struct tm* sys_time = localtime(&t_sec);
	struct tm t = *sys_time;
	va_list va_list;

	// �����ǰ����ά���������͵�ǰ������ͬ������־�����������־������Ҫ�����µ���־�ļ�
	if (now_day_ != t.tm_mday || line_count_ % MAX_LINES_ == 0) {
		std::unique_lock<std::mutex> locker(mtx_);
		locker.unlock();

		// �����µ�tail
		char tail[36] = { 0 };
		snprintf(tail, 36, "%04d_%02d_%02d", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday);

		// �����µ���־�ļ�������
		char new_file[LOG_NAME_LEN];
		if (now_day_ != t.tm_mday) {
			// �������Ϊʱ��Բ��ϣ�������log,���´���ʱ�������
			snprintf(new_file, LOG_NAME_LEN - 72, "%s/%s%s", path_, tail, suffix_);
			now_day_ = t.tm_mday;
			line_count_ = 0;
		}
		else {
			// ������־�ļ�������������������������Ϊ20�У�����ǰ30�У���Ҫ��������ļ�
			snprintf(new_file, LOG_NAME_LEN - 72, "%s/%s-%d%s", path_, tail, (line_count_ / MAX_LINES_), suffix_);
		}

		locker.lock();
		flush();
		fp_ = fopen(new_file, "a");


	}
	// ����Ҫ�������ļ����ڵ�ǰ��־�ļ���д
	{
		std::unique_lock<std::mutex> locker(mtx_);
		line_count_++;
		// ȷ��Ҫд�����ݣ��ٸ����Ƿ��첽�������д�뵽�ļ���
		// ��־�������ʱ�䣬��д����buff_�У��ٷŵ���������
		int n = snprintf(buff_.beginWritePtrNoConst(), 128, "%d-%02d-%02d %02d:%02d:%02d.%06ld ", t.tm_year + 1900, t.tm_mon + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, now.tv_usec);
		// �ƶ�write_pos_
		buff_.hasWritten(n);
		// �����־ͷ
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
		// �������format������ֵ��va_list
		va_start(va_list, format);
		// va_list��һ��ɱ��������va_list���滻��format�е�һЩ��ʽ���ַ�������д��buff_��
		int m = vsnprintf(buff_.beginWritePtrNoConst(), buff_.writeableBytes(), format, va_list);
		va_end(va_list);

		buff_.hasWritten(m);
		buff_.append("\n\0", 2);
		// ������buff_���Ѿ��洢��������־����
		if (is_async_ && !block_queue_ptr_->full()) {
			// �첽����buff_�е����ݷŵ������У������첽д�߳�ȥд���ļ���
			std::string str(buff_.beginReadPtr(), buff_.readableBytes());
			buff_.retrieveAll();
			block_queue_ptr_->push_back(str);
		}
		else {
			// ͬ����ֱ�������ｫ��־д���ļ��У���������
			fputs(buff_.beginReadPtr(), fp_);
		}
	
	}
}

// ��д�̵߳��õķ���
void Log::flushLogThread() {
	Log::Instance()->asyncWrite();
}

// �첽д���Ӷ�����ȡ���������־����д�뵽�ļ���
// ��д�̸߳ɵ���
void Log::asyncWrite() {
	std::string str = "";
	while (block_queue_ptr_->pop(str)) {
		std::lock_guard<std::mutex> locker(mtx_);
		// fputs��strд��fp_�У�����־д���ļ����������fputs
		fputs(str.c_str(), fp_);
	}

}


void Log::flush() {
	
	if (is_async_) {
		block_queue_ptr_->flush();
	}
	// ǿ�Ƚ�������������д��fp_ָ���ļ���
	fflush(fp_);

}


int Log::getLevel() {
	std::lock_guard<std::mutex> locker(mtx_);
	return level_;
}

