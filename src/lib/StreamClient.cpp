#include "Rasheeq.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

R::StreamClient::StreamClient(PollerPool& pool, StreamServer& server, int fd):
	fd_(fd),
	poller_(NULL),
	server_(&server),
	userData_(NULL),
	onConnect_(),
	onDestruct_(),
	onDisconnect_()
{
	int v = ::fcntl(this->fd_, F_GETFL, 0);
	::fcntl(this->fd_, F_SETFL, v | O_NONBLOCK);
	#warning Stubbed
	this->poller_ = pool.add(this->fd_, [](Poller& poller, int fd, void* arg)->bool { return true; }, [](Poller& poller, int fd, void* arg)->bool { return true; });
};

R::StreamClient::StreamClient(StreamClient&& move):
	fd_(move.fd_),
	server_(move.server_),
	userData_(move.userData_),
	onConnect_(std::move(move.onConnect_)),
	onDestruct_(std::move(move.onDestruct_)),
	onDisconnect_(std::move(move.onDisconnect_))
{
	move.fd_ = -1;
	move.poller_ = NULL;
};

R::StreamClient::~StreamClient() {
	if (this->fd_ != -1) {
		this->poller_->remove(this->fd_);
		this->fd_ = -1;
		for (auto it = this->onDisconnect_.begin(); it != this->onDisconnect_.end(); ++it)
			(*it)(*this);
		if (this->server_ != NULL)
			this->server_->clientDisconnected_(*this);
	}
	if (this->poller_ != NULL) {
		for (auto it = this->onDestruct_.begin(); it != this->onDestruct_.end(); ++it)
			(*it)(*this);
	}
};

void R::StreamClient::disconnect() {
	if (this->fd_ != -1) {
		this->poller_->remove(this->fd_);
		this->fd_ = -1;
		for (auto it = this->onDisconnect_.begin(); it != this->onDisconnect_.end(); ++it)
			(*it)(*this);
		if (this->server_ != NULL)
			this->server_->clientDisconnected_(*this);
	}
};

