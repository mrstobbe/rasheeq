#ifndef Rasheeq_Poller_H_
#define Rasheeq_Poller_H_

#ifdef __cplusplus

#include <functional>
#include <unordered_map>

namespace R {

class Poller {
	public:
		enum Events {
			peRead = 0x01,
			peWrite = 0x02,
			peError = 0x04
		}; //enum Events
		typedef std::function<bool(int, void* userArg)> ReadReady;
		typedef std::function<bool(int, void* userArg)> WriteReady;
		typedef std::function<void(int, void* userArg)> ErrorOccurred;
	protected:
		struct Entry {
			int fd;
			int events;
			ReadReady onRead;
			WriteReady onWrite;
			ErrorOccurred onError;
			void* userArg;
		}; //struct Entry
	private:
		#if RASHEEQ_HAVE_EPOLL
		int efd_;
		#endif
		std::unordered_map<int, Entry> entries_;
		long timeout_;
	public:
		Poller();
		~Poller();
	public:
		bool add(int fd, ReadReady onReadReady, WriteReady onWriteReady);
		bool add(int fd, ReadReady onReadReady, WriteReady onWriteReady, ErrorOccurred onError);
		bool add(int fd, ReadReady onReadReady, WriteReady onWriteReady, void* userArg);
		bool add(int fd, ReadReady onReadReady, WriteReady onWriteReady, ErrorOccurred onError, void* userArg);
		inline void poll() { this->poll(this->timeout_); };
		void poll(const long timeout);
		bool remove(int fd);
	private:
		bool add(int fd, ReadReady& onReadReady, WriteReady& onWriteReady, ErrorOccurred* onError, void* userArg);
}; //class Poller

}; //ns R

extern "C" {
#endif /* __cplusplus */

typedef struct rasheeq_poller { } rasheeq_poller_t;

typedef int(*rasheeq_readready_callback)(int fd, void* user_arg);
typedef int(*rasheeq_writeready_callback)(int fd, void* user_arg);
typedef void(*rasheeq_erroroccured_callback)(int fd, void* user_arg);

int rasheeq_poller_add(rasheeq_poller_t* poller, int fd, rasheeq_readready_callback onreadready, rasheeq_writeready_callback onwriteready, rasheeq_erroroccured_callback onerror, void* user_arg);
rasheeq_poller_t* poller_create();
void poller_destroy(rasheeq_poller_t* poller);
void rasheeq_poller_poll(rasheeq_poller_t* poller);
void rasheeq_poller_timed_poll(rasheeq_poller_t* poller, const long timeout);
int rasheeq_poller_remove(rasheeq_poller_t* poller, int fd);

#ifdef __cplusplus
} //extern "C"
#endif /* __cplusplus */


#endif /* Rasheeq_Poller_H_ */
