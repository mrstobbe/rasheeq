#include "Rasheeq/Poller.h"

R::Poller::Poller():
	#if RASHEEQ_HAVE_EPOLL
	efd_(-1),
	#endif
	timeout_(0)
{
	#warning Poller::Poller() stubbed
};

R::Poller::~Poller() {
	#warning Poller::~Poller() stubbed
};

void R::Poller::poll(const long timeout) {
	#warning Poller::poll() stubbed
};

extern "C" {

void rasheeq_poller_poll(R::Poller* poller) {
	poller->poll();
};

void rasheeq_poller_timed_poll(R::Poller* poller, const long timeout) {
	poller->poll(timeout);
};

}; //extern "C"

