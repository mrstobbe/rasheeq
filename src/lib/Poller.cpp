#include "Rasheeq/Poller.h"

#include <unistd.h>

#if RASHEEQ_HAVE_EPOLL
	#include <sys/epoll.h>
#endif /* RASHEEQ_HAVE_EPOLL */

R::Poller::Poller():
	#if RASHEEQ_HAVE_EPOLL
	efd_(-1),
	#endif /* RASHEEQ_HAVE_EPOLL */
	entries_(),
	timeout_(0)
{
	#if RASHEEQ_HAVE_EPOLL
	//#TODO: Test for errors
	this->efd = ::epoll_create1(0);
	#endif /* RASHEEQ_HAVE_EPOLL */
};

R::Poller::~Poller() {
	for (auto it = this->entries_.begin(); it != this->entries_.end(); ++it) {
		//#TODO: Support "close" event trigger
		::close(it->first);
	}
	#if RASHEEQ_HAVE_EPOLL
	::close(this->efd);
	this->efd = -1;
	#endif /* RASHEEQ_HAVE_EPOLL */
};

bool R::Poller::add(int fd, ReadReady onReadReady, WriteReady onWriteReady) {
	//#TODO: Sanity check -1
	return this->add(fd, onReadReady, onWriteReady, NULL, NULL);
};

bool R::Poller::add(int fd, ReadReady onReadReady, WriteReady onWriteReady, ErrorOccurred onError) {
	//#TODO: Sanity check -1
	return this->add(fd, onReadReady, onWriteReady, &onError, NULL);
};

bool R::Poller::add(int fd, ReadReady onReadReady, WriteReady onWriteReady, void* userArg) {
	//#TODO: Sanity check -1
	return this->add(fd, onReadReady, onWriteReady, NULL, userArg);
};

bool R::Poller::add(int fd, ReadReady onReadReady, WriteReady onWriteReady, ErrorOccurred onError, void* userArg) {
	//#TODO: Sanity check -1
	return this->add(fd, onReadReady, onWriteReady, &onError, userArg);
};

void R::Poller::poll(const long timeout) {
	#warning Poller::poll() stubbed
};

bool R::Poller::remove(int fd) {
	auto it = this->entries_.find(fd);
	if (it == this->entries_.end())
		return false;
	//#TODO: Support "close" event trigger
	this->entries_.erase(it);
	::close(fd);
	return true;
};


bool R::Poller::add(int fd, ReadReady& onReadReady, WriteReady& onWriteReady, ErrorOccurred* onError, void* userArg) {
	int events = peRead | peWrite;
	if (onError != NULL)
		events |= peError;
	auto it = this->entries_.find(fd);
	if (it != this->entries_.end()) {
		Entry& entry = it->second;
		if (events == entry.events)
			return false;
		//#TODO: Support modification (via Entry mod and epoll_ctl as needed)
		#warning Poller::add() -- Modification of pre-existing poller entry not yet supported
		return true;
	}
	#if RASHEEQ_HAVE_EPOLL
	uint32_t ep_events = EPOLLET | EPOLLIN | EPOLLOUT;
	if (onError != NULL)
		ep_events |= EPOLLET;
	::epoll_event ep_event = {
			events: ep_events,
			data: { fd: fd }
		};
	//#TODO: Test for errors
	::epoll_ctl(this->efd, EPOLL_CTL_ADD, entry.fd, &ep_event);
	#endif /* RASHEEQ_HAVE_EPOLL */
	Entry& entry = this->entries_[fd];
	entry.fd = fd;
	entry.events = events;
	entry.onRead = onReadReady;
	entry.onWrite = onWriteReady;
	entry.onError = (onError != NULL) ? *onError : ErrorOccurred();
	entry.userArg = userArg;
	return true;
};


extern "C" {

void rasheeq_poller_poll(R::Poller* poller) {
	poller->poll();
};

void rasheeq_poller_timed_poll(R::Poller* poller, const long timeout) {
	poller->poll(timeout);
};

}; //extern "C"

