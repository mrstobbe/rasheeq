#include "Rasheeq/Poller.h"

#include <cerrno>
#include <unistd.h>

#if RASHEEQ_HAVE_EPOLL
	#include <sys/epoll.h>
#endif /* RASHEEQ_HAVE_EPOLL */

int R::Poller::maxPerPoll = 64;

R::Poller::Poller():
	#if RASHEEQ_HAVE_EPOLL
		efd_(-1),
	#endif /* RASHEEQ_HAVE_EPOLL */
	entries_(),
	haveEvent_(),
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

void R::Poller::poll(const int timeout) {
	for (auto it = this->haveEvent_.begin(); it != this->haveEvent_.end(); ++it) {
		Entry& entry = this->entries_[*it];
		if (((entry.activeEvents & peWrite) != 0) && (entry.onWrite(entry.fd, entry.userArg)))
			entry.activeEvents ^= peWrite;
		if (((entry.activeEvents & peRead) != 0) && (entry.onRead(entry.fd, entry.userArg)))
			entry.activeEvents ^= peRead;
		if (entry.activeEvents == 0)
			this->haveEvent_.erase(it);
	}

	#if RASHEEQ_HAVE_EPOLL
		::epoll_event events[R::Poller::maxPerPoll];
		int res = ::epoll_wait(this->efd_, events, R::Poller::maxPerPoll, timeout);
		if ((res == 0) || ((res == -1) && (errno == EINTR)))
			return;
		if (res == -1) {
			//#TODO: Handle unexpected errors
			return;
		}
		for (int i = 0; i < res; ++i) {
			#if RASHEEQ_DEBUG_BUILD
				//Same complexity as the release version, but with branching.
				//None of these if conditions should evalutate as true, so these are
				//really assertions for test purposes.
				auto eit = this->entries_.find(events[i].data.fd);
				if ((eit == this->entries_.end()) || (((events[i].events & EPOLLERR) != 0) && ((peError & entry.events) == 0))) {
					//#TODO: Fatal here
					continue;
				}
				Entry& entry = eit->second;
			#else
				Entry& entry = this->entries_[events[i].data.fd];
			#endif /* RASHEEQ_DEBUG_BUILD */
			int oactive = entry.activeEvents;
			if (((events[i].events & EPOLLOUT) != 0) && (!entry.onWrite(entry.fd, entry.userArg)))
				entry.activeEvents |= peWrite;
			if (((events[i].events & EPOLLIN) != 0) && (!entry.onRead(entry.fd, entry.userArg)))
				entry.activeEvents |= peRead;
			if (entry.activeEvents != oactive)
				this->haveEvent_.insert(entry.fd);
		}
	#else
		//#TODO: kpoll/win32 api/fallback on select(), etc
	#endif /* RASHEEQ_HAVE_EPOLL */
};

bool R::Poller::remove(int fd) {
	auto it = this->entries_.find(fd);
	if (it == this->entries_.end())
		return false;
	this->haveEvent_.erase(fd);
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
	if (!isMod)
		entry.activeEvents = 0;
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

void rasheeq_poller_timed_poll(rasheeq_poller_t* poller, const int timeout) {
	reinterpret_cast<R::Poller*>(poller)->poll(timeout);
};

int rasheeq_poller_remove(rasheeq_poller_t* poller, int fd) {
	return (reinterpret_cast<R::Poller*>(poller)->remove(fd)) ? 1 : 0;
};


}; //extern "C"

