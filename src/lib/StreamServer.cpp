#include "Rasheeq.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#if RASHEEQ_HAVE_SCTP
	#include <netinet/sctp.h>
#endif
#include <fcntl.h>
#include <unistd.h>

R::StreamServer::StreamServer(R::PollerPool& pool):
	clients_(),
	fd_(-1),
	greedy_(false),
	listenAddr_(),
	poller_(NULL),
	pool_(&pool),
	reapReady_(),
	userData_(NULL),
	onClientConnect_(),
	onClientDisconnect_(),
	onDestruct_()
{
};

R::StreamServer::StreamServer(StreamServer&& move):
	clients_(std::move(move.clients_)),
	fd_(move.fd_),
	greedy_(move.greedy_),
	listenAddr_(std::move(move.listenAddr_)),
	poller_(move.poller_),
	pool_(move.pool_),
	reapReady_(std::move(move.reapReady_)),
	userData_(move.userData_),
	onClientConnect_(std::move(move.onClientConnect_)),
	onClientDisconnect_(std::move(move.onClientDisconnect_)),
	onDestruct_(std::move(move.onDestruct_))
{
	move.fd_ = -1;
};

R::StreamServer::~StreamServer() {
	if (this->fd_ != -1) {
		if (this->reapReady_.size() != 0) {
			for (auto it = this->reapReady_.begin(); it != this->reapReady_.end(); ++it)
				delete *it;
			this->reapReady_.clear();
		}

		for (auto it = this->clients_.begin(); it != this->clients_.end(); it = this->clients_.begin())
			(*it)->disconnect();
		for (auto it = this->onDestruct_.begin(); it != this->onDestruct_.end(); ++it)
			(*it)(*this);
		this->poller_->remove(this->fd_);
		this->fd_ = -1;
	}
};

void R::StreamServer::listen(const Net::StreamAddr& address) {
	this->bind_(address);
	this->listenAddr_ = address;
	this->listen_();
};

void R::StreamServer::listen(const Net::StreamAddr& address, const int backlog) {
	this->bind_(address);
	this->listenAddr_ = address;
	this->listen_(backlog);
};

void R::StreamServer::onClientConnect(const ClientConnected& callback) {
	this->onClientConnect_.push_back(callback);
};

void R::StreamServer::onClientDisconnect(const ClientDisconnected& callback) {
	this->onClientDisconnect_.push_back(callback);
};

void R::StreamServer::onDestruct(const Destructing& callback) {
	this->onDestruct_.push_back(callback);
};

#include <cstdio>

void R::StreamServer::bind_(const Net::StreamAddr& address) {
	this->fd_ = ::socket(address.nativeAddrFamily(), address.nativeDomain(), address.nativeProtocol());
	if (this->fd_ == -1) {
		perror("socket()");
		return;
	}
	int v = ::fcntl(this->fd_, F_GETFL, 0);
	if (v == -1) {
		perror("fcntl()");
		return;
	}
	if (::fcntl(this->fd_, F_SETFL, v | O_NONBLOCK) == -1) {
		perror("fcntl(set)");
		return;
	}
	v = 1;
	if (::setsockopt(this->fd_, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v)) == -1) {
		perror("setsockopt(SO_REUSEADDR)");
		return;
	}
	if (::bind(this->fd_, reinterpret_cast<const sockaddr*>((void*)address), (size_t)address) == -1) {
		perror("bind()");
		return;
	}
	this->poller_ = this->pool_->add(this->fd_, onReadReady_, onWriteReady_, this);
};

void R::StreamServer::listen_() {
	if (::listen(this->fd_, SOMAXCONN) == -1) {
		perror("listen()");
		return;
	}
};

void R::StreamServer::listen_(const int backlog) {
	if (::listen(this->fd_, backlog) == -1) {
		perror("listen()");
		return;
	}
};

void R::StreamServer::clientDisconnected_(StreamClient& client) {
	auto it = this->clients_.find(&client);
	if (it != this->clients_.end()) {
		for (auto cit = this->onClientDisconnect_.begin(); cit != this->onClientDisconnect_.end(); ++cit)
			(*cit)(*this, client);
		R::StreamClient* client = *it;
		this->clients_.erase(it);
		this->reapReady_.insert(client);
	}
};

bool R::StreamServer::onReadReady_(Poller& poller, int fd, void* arg) {
	StreamServer* server = reinterpret_cast<StreamServer*>(arg);
	if (server->reapReady_.size() != 0) {
		for (auto it = server->reapReady_.begin(); it != server->reapReady_.end(); ++it)
			delete *it;
		server->reapReady_.clear();
	}
	while (true) {
		int cliFD = ::accept(fd, NULL, NULL);
		if (cliFD == -1) {
			//#TODO: Test for actual error (okay: EINTR, EAGAIN, or EWOULDBLOCK)
			return true;
		}
		StreamClient* client = new StreamClient(*(server->pool_), *server, cliFD);
		server->clients_.insert(client);
		for (auto it = server->onClientConnect_.begin(); it != server->onClientConnect_.end(); ++it)
			(*it)(*server, *client);
		if (!server->greedy_)
			break;
	}
	return false;
};

bool R::StreamServer::onWriteReady_(Poller& poller, int fd, void* arg) {
	return true;
};


