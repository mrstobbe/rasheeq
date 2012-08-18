#include "Rasheeq.h"

#include <cstdlib>
#include <sys/time.h>

R::PollerPool::PollerPool():
	pollers_(),
	randSeed_(),
	timeout_(0)
{
	timeval tv;
	::gettimeofday(&tv, NULL);
	this->randSeed_ = tv.tv_usec;
};

R::PollerPool::PollerPool(const int timeout):
	pollers_(),
	randSeed_(),
	timeout_(timeout)
{
	timeval tv;
	::gettimeofday(&tv, NULL);
	this->randSeed_ = tv.tv_usec;
};

R::PollerPool::~PollerPool() {
	for (auto it = this->pollers_.begin(); it != this->pollers_.end(); ++it)
		delete *it;
};

R::Poller& R::PollerPool::add(int fd, Poller::ReadReady onReadReady, Poller::WriteReady onWriteReady) {
	return this->add(fd, onReadReady, onWriteReady, NULL, NULL);
};

R::Poller& R::PollerPool::add(int fd, Poller::ReadReady onReadReady, Poller::WriteReady onWriteReady, Poller::ErrorOccurred onError) {
	return this->add(fd, onReadReady, onWriteReady, &onError, NULL);
};

R::Poller& R::PollerPool::add(int fd, Poller::ReadReady onReadReady, Poller::WriteReady onWriteReady, void* userArg) {
	return this->add(fd, onReadReady, onWriteReady, NULL, userArg);
};

R::Poller& R::PollerPool::add(int fd, Poller::ReadReady onReadReady, Poller::WriteReady onWriteReady, Poller::ErrorOccurred onError, void* userArg) {
	return this->add(fd, onReadReady, onWriteReady, &onError, userArg);
};

R::Poller& R::PollerPool::createPoller() {
	R::Poller* res = new Poller(this->timeout_);
	this->pollers_.insert(res);
	return *res;
};

R::Poller& R::PollerPool::createPoller(const int timeout) {
	R::Poller* res = new Poller(timeout);
	this->pollers_.insert(res);
	return *res;
};

R::Poller& R::PollerPool::add(int fd, Poller::ReadReady& onReadReady, Poller::WriteReady& onWriteReady, Poller::ErrorOccurred* onError, void* userArg) {
	//#TODO: Replace rand_r() with a better/faster PRNG (aka, implement ISAAC)
	//worst: O(n), avg: O(n/2), best: O(1)
	Poller* poller = NULL;
	size_t n = this->pollers_.size();
	for (auto it = this->pollers_.begin(); it != this->pollers_.end(); ++it) {
		int x = 1 + (int)(n * (::rand_r(&this->randSeed_) / (static_cast<double>(RAND_MAX) + 1)));
		if (x == 1) {
			poller = *it;
			break;
		}
		--n;
	}
	if (onError != NULL) {
		poller->add(fd, onReadReady, onWriteReady, *onError, userArg);
	} else {
		poller->add(fd, onReadReady, onWriteReady, userArg);
	}
	return *poller;
};

