#include "Rasheeq.h"

#include <cerrno>
#include <unistd.h>

#if RASHEEQ_HAVE_EPOLL
	#include <sys/epoll.h>
#endif /* RASHEEQ_HAVE_EPOLL */

int R::Poller::maxPerPoll = 64;

R::Poller::Poller():
	addReady_(),
	#if RASHEEQ_HAVE_EPOLL
		efd_(-1),
	#endif /* RASHEEQ_HAVE_EPOLL */
	entries_(),
	haveEvent_(),
	polling_(false),
	reapReady_(),
	timeout_(0)
{
	#if RASHEEQ_HAVE_EPOLL
		//#TODO: Test for errors
		this->efd_ = ::epoll_create1(0);
	#endif /* RASHEEQ_HAVE_EPOLL */
};

R::Poller::Poller(const int timeout):
	addReady_(),
	#if RASHEEQ_HAVE_EPOLL
		efd_(-1),
	#endif /* RASHEEQ_HAVE_EPOLL */
	entries_(),
	haveEvent_(),
	reapReady_(),
	timeout_(timeout)
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

int R::Poller::timeout() const {
	return this->timeout_;
};

void R::Poller::timeout(const int value) {
	this->timeout_ = value;
};

bool R::Poller::add(int fd, Added onAdded, ReadReady onReadReady, WriteReady onWriteReady) {
	//#TODO: Sanity check -1
	return this->add(fd, onAdded, onReadReady, onWriteReady, NULL, NULL);
};

bool R::Poller::add(int fd, Added onAdded, ReadReady onReadReady, WriteReady onWriteReady, ErrorOccurred onError) {
	//#TODO: Sanity check -1
	return this->add(fd, onAdded, onReadReady, onWriteReady, &onError, NULL);
};

bool R::Poller::add(int fd, Added onAdded, ReadReady onReadReady, WriteReady onWriteReady, void* userArg) {
	//#TODO: Sanity check -1
	return this->add(fd, onAdded, onReadReady, onWriteReady, NULL, userArg);
};

bool R::Poller::add(int fd, Added onAdded, ReadReady onReadReady, WriteReady onWriteReady, ErrorOccurred onError, void* userArg) {
	//#TODO: Sanity check -1
	return this->add(fd, onAdded, onReadReady, onWriteReady, &onError, userArg);
};

void R::Poller::poll(const int timeout) {
	this->polling_ = true;
	if (this->addReady_.size() != 0) {
		for (auto it = this->addReady_.begin(); it != this->addReady_.end(); ++it) {
			this->entries_[it->first] = it->second;
			#if RASHEEQ_HAVE_EPOLL
				uint32_t ep_events = EPOLLET | EPOLLIN | EPOLLOUT;
				if ((it->second.events & peError) != 0)
					ep_events |= EPOLLERR;
				::epoll_event ep_event = {
						events: ep_events,
						data: { fd: it->first }
					};
				//#TODO: Test for errors
				if (::epoll_ctl(this->efd_, EPOLL_CTL_ADD, it->first, &ep_event) == -1)
					perror("epoll_ctl()");
			#endif /* RASHEEQ_HAVE_EPOLL */
			it->second.onAdded(*this, it->first, it->second.userArg);
		}
		this->addReady_.clear();
	}

	if (this->haveEvent_.size() != 0) {
		std::unordered_set<int> reap;
		for (auto it = this->haveEvent_.begin(); it != this->haveEvent_.end(); ++it) {
			Entry& entry = this->entries_[*it];
			if (((entry.activeEvents & peWrite) != 0) && (entry.onWrite(*this, entry.fd, entry.userArg)))
				entry.activeEvents ^= peWrite;
			if (((entry.activeEvents & peRead) != 0) && (entry.onRead(*this, entry.fd, entry.userArg)))
				entry.activeEvents ^= peRead;
			if (entry.activeEvents == 0)
				reap.insert(*it);
				//it = this->haveEvent_.erase(it);
		}
		for (auto it = reap.begin(); it != reap.end(); ++it)
			this->haveEvent_.erase(*it);
	}

	#if RASHEEQ_HAVE_EPOLL
		::epoll_event events[R::Poller::maxPerPoll];
		int res = ::epoll_wait(this->efd_, events, R::Poller::maxPerPoll, timeout);
		if ((res == 0) || ((res == -1) && (errno == EINTR))) {
			this->polling_ = false;
			return;
		} else if (res == -1) {
			//#TODO: Handle unexpected errors
			this->polling_ = false;
			return;
		}
		for (int i = 0; i < res; ++i) {
			int fd = events[i].data.fd;
			Entry& entry = this->entries_[fd];
			if (((events[i].events & EPOLLERR) != 0) && ((entry.events & peError) != 0)) {
				entry.onError(*this, entry.fd, entry.userArg);
				if (this->reapReady_.find(events[i].data.fd) != this->reapReady_.end())
					continue;
			}
			int oactive = entry.activeEvents;
			if (((events[i].events & EPOLLOUT) != 0) && (!entry.onWrite(*this, entry.fd, entry.userArg)))
				entry.activeEvents |= peWrite;
			if (((events[i].events & EPOLLIN) != 0) && (!entry.onRead(*this, entry.fd, entry.userArg)))
				entry.activeEvents |= peRead;
			if (entry.activeEvents != oactive)
				this->haveEvent_.insert(entry.fd);
		}
	#else
		//#TODO: kpoll/win32 api/fallback on select(), etc
	#endif /* RASHEEQ_HAVE_EPOLL */
	this->polling_ = false;

	if (this->reapReady_.size() != 0) {
		for (auto it = this->reapReady_.begin(); it != this->reapReady_.end(); ++it) {
			this->haveEvent_.erase(*it);
			this->entries_.erase(*it);
		}
		this->reapReady_.clear();
	}
};

bool R::Poller::remove(int fd) {
	auto it = this->entries_.find(fd);
	if ((it == this->entries_.end()) || (this->reapReady_.find(fd) != this->reapReady_.end()))
		return false;
	::close(fd);
	if (this->polling_) {
		this->reapReady_.insert(fd);
	} else {
		this->haveEvent_.erase(fd);
		this->entries_.erase(fd);
	}
	return true;
};


bool R::Poller::add(int fd, Added& onAdded, ReadReady& onReadReady, WriteReady& onWriteReady, ErrorOccurred* onError, void* userArg) {
	/*
	if ((this->entries_.find(fd) != this->entries_.end()) || (this->addReady_.find(fd) != this->addReady_.end()))
		return false;
	*/
	int events = peRead | peWrite;
	if (onError != NULL)
		events |= peError;
	Entry& entry = this->addReady_[fd];
	entry.fd = fd;
	entry.events = events;
	entry.activeEvents = 0;
	entry.onAdded = onAdded;
	entry.onRead = onReadReady;
	entry.onWrite = onWriteReady;
	entry.onError = (onError != NULL) ? *onError : ErrorOccurred();
	entry.userArg = userArg;
	return true;
};


extern "C" {

int rasheeq_poller_add(rasheeq_poller_t* poller, int fd, rasheeq_added_callback onadded, rasheeq_readready_callback onreadready, rasheeq_writeready_callback onwriteready, rasheeq_erroroccured_callback onerror, void* user_arg) {
	R::Poller* p = reinterpret_cast<R::Poller*>(poller);
	if (onerror != NULL)
		return (p->add(fd, reinterpret_cast<void(*)(R::Poller&, int, void*)>(onadded), reinterpret_cast<int(*)(R::Poller&, int, void*)>(onreadready), reinterpret_cast<int(*)(R::Poller&, int, void*)>(onwriteready), reinterpret_cast<void(*)(R::Poller&, int, void*)>(onerror), user_arg)) ? 1 : 0;
	return (p->add(fd, reinterpret_cast<void(*)(R::Poller&, int, void*)>(onadded), reinterpret_cast<int(*)(R::Poller&, int, void*)>(onreadready), reinterpret_cast<int(*)(R::Poller&, int, void*)>(onwriteready), user_arg)) ? 1 : 0;
};

rasheeq_poller_t* rasheeq_poller_create() {
	return reinterpret_cast<rasheeq_poller_t*>(new R::Poller());
};

void poller_destroy(rasheeq_poller_t* poller) {
	delete reinterpret_cast<R::Poller*>(poller);
};

void rasheeq_poller_poll(rasheeq_poller_t* poller) {
	reinterpret_cast<R::Poller*>(poller)->poll();
};

int rasheeq_poller_remove(rasheeq_poller_t* poller, int fd) {
	return (reinterpret_cast<R::Poller*>(poller)->remove(fd)) ? 1 : 0;
};

int rasheeq_poller_timeout_get(rasheeq_poller_t* poller) {
	return reinterpret_cast<R::Poller*>(poller)->timeout();
};

int rasheeq_poller_timeout_set(rasheeq_poller_t* poller, const int timeout) {
	int res = reinterpret_cast<R::Poller*>(poller)->timeout();
	reinterpret_cast<R::Poller*>(poller)->timeout(timeout);
	return res;
};

void rasheeq_poller_timed_poll(rasheeq_poller_t* poller, const int timeout) {
	reinterpret_cast<R::Poller*>(poller)->poll(timeout);
};


}; //extern "C"

