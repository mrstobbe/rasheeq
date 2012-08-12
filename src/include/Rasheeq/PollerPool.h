#ifndef Rasheeq_PollerPool_H_
#define Rasheeq_PollerPool_H_

#if __cplusplus

#include <unordered_set>

namespace R {

class PollerPool {
	protected:
		std::unordered_set<Poller*> pollers_;
		unsigned int randSeed_;
	public:
		PollerPool();
		~PollerPool();
	public:
		Poller* add(int fd, Poller::ReadReady onReadReady, Poller::WriteReady onWriteReady);
		Poller* add(int fd, Poller::ReadReady onReadReady, Poller::WriteReady onWriteReady, Poller::ErrorOccurred onError);
		Poller* add(int fd, Poller::ReadReady onReadReady, Poller::WriteReady onWriteReady, void* userArg);
		Poller* add(int fd, Poller::ReadReady onReadReady, Poller::WriteReady onWriteReady, Poller::ErrorOccurred onError, void* userArg);
		Poller* createPoller();
	private:
		Poller* add(int fd, Poller::ReadReady& onReadReady, Poller::WriteReady& onWriteReady, Poller::ErrorOccurred* onError, void* userArg);
	private:
		PollerPool(const PollerPool& copy) { };
		PollerPool& operator =(const PollerPool& copy) { return *this; };
};


}; //ns R

extern "C" {
#endif /* __cplusplus */



#if __cplusplus
}; //extern "C"
#endif /* __cplusplus */

#endif /* Rasheeq_PollerPool_H_ */
