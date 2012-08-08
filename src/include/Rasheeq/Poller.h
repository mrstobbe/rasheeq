#ifndef Rasheeq_Poller_H_
#define Rasheeq_Poller_H_

#ifdef __cplusplus

#include <functional>
#include <unordered_map>
#include <unordered_set>

namespace R {

class Poller {
	public:
		static int maxPerPoll;
	public:
		enum Events {
			peRead = 0x01,
			peWrite = 0x02,
			peError = 0x04
		}; //enum Events
		typedef std::function<bool(int, void*)> ReadReady;
		typedef std::function<bool(int, void*)> WriteReady;
		typedef std::function<void(int, void*)> ErrorOccurred;
	protected:
		struct Entry {
			int fd;
			int events;
			int activeEvents;
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
		std::unordered_set<int> haveEvent_;
		int timeout_;
	public:
		Poller();
		Poller(const int timeout);
		~Poller();
	public:
		int timeout() const;
		void timeout(const int value);
	public:
		bool add(int fd, ReadReady onReadReady, WriteReady onWriteReady);
		bool add(int fd, ReadReady onReadReady, WriteReady onWriteReady, ErrorOccurred onError);
		bool add(int fd, ReadReady onReadReady, WriteReady onWriteReady, void* userArg);
		bool add(int fd, ReadReady onReadReady, WriteReady onWriteReady, ErrorOccurred onError, void* userArg);
		inline void poll() { this->poll(this->timeout_); };
		void poll(const int timeout);
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
void rasheeq_poller_destroy(rasheeq_poller_t* poller);
void rasheeq_poller_poll(rasheeq_poller_t* poller);
int rasheeq_poller_remove(rasheeq_poller_t* poller, int fd);
void rasheeq_poller_timed_poll(rasheeq_poller_t* poller, const int timeout);
int rasheeq_poller_timeout_get(rasheeq_poller_t* poller);
int rasheeq_poller_timeout_set(rasheeq_poller_t* poller, const int timeout);

#ifdef __cplusplus
} //extern "C"
#endif /* __cplusplus */


#endif /* Rasheeq_Poller_H_ */
