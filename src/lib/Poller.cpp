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
	this->efd_ = ::epoll_create1(0);
	#endif /* RASHEEQ_HAVE_EPOLL */
};

R::Poller::~Poller() {
	for (auto it = this->entries_.begin(); it != this->entries_.end(); ++it) {
		//#TODO: Support "close" event trigger
		::close(it->first);
	}
	#if RASHEEQ_HAVE_EPOLL
	::close(this->efd_);
	this->efd_ = -1;
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
	bool isMod = (it != this->entries_.end());
	if ((isMod) && (events == it->second.events))
		return false;
	#if RASHEEQ_HAVE_EPOLL
	uint32_t ep_events = EPOLLET | EPOLLIN | EPOLLOUT;
	if (onError != NULL)
		ep_events |= EPOLLERR;
	::epoll_event ep_event = {
			events: ep_events,
			data: { fd: fd }
		};
	//#TODO: Test for errors
	::epoll_ctl(this->efd_, (isMod) ? EPOLL_CTL_MOD : EPOLL_CTL_ADD, fd, &ep_event);
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

int rasheeq_poller_add(rasheeq_poller_t* poller, int fd, rasheeq_readready_callback onreadready, rasheeq_writeready_callback onwriteready, rasheeq_erroroccured_callback onerror, void* user_arg) {
	R::Poller* p = reinterpret_cast<R::Poller*>(poller);
	if (onerror != NULL)
		return (p->add(fd, onreadready, onwriteready, onerror, user_arg)) ? 1 : 0;
	return (p->add(fd, onreadready, onwriteready, user_arg)) ? 1 : 0;
};

rasheeq_poller_t* poller_create() {
	return reinterpret_cast<rasheeq_poller_t*>(new R::Poller());
};

void poller_destroy(rasheeq_poller_t* poller) {
	delete reinterpret_cast<R::Poller*>(poller);
};

void rasheeq_poller_poll(rasheeq_poller_t* poller) {
	reinterpret_cast<R::Poller*>(poller)->poll();
};

void rasheeq_poller_timed_poll(rasheeq_poller_t* poller, const long timeout) {
	reinterpret_cast<R::Poller*>(poller)->poll(timeout);
};

int rasheeq_poller_remove(rasheeq_poller_t* poller, int fd) {
	return (reinterpret_cast<R::Poller*>(poller)->remove(fd)) ? 1 : 0;
};


}; //extern "C"

