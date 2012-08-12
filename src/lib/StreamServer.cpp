#include "Rasheeq.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

R::StreamServer::StreamServer(R::PollerPool& pool):
	clients_(),
	fd_(-1),
	greedy_(false),
	poller_(NULL),
	pool_(&pool),
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
	poller_(move.poller_),
	pool_(move.pool_),
	userData_(move.userData_),
	onClientConnect_(std::move(move.onClientConnect_)),
	onClientDisconnect_(std::move(move.onClientDisconnect_)),
	onDestruct_(std::move(move.onDestruct_))
{
	move.fd_ = -1;
};

R::StreamServer::~StreamServer() {
	if (this->fd_ != -1) {
		for (auto it = this->onDestruct_.begin(); it != this->onDestruct_.end(); ++it)
			(*it)(*this);
		this->poller_->remove(this->fd_);
		this->fd_ = -1;
	}
};

void R::StreamServer::listen(const int port) {
	::sockaddr_in addr = {
			sin_family: AF_INET,
			sin_port: htons(port),
			sin_addr: { s_addr: 0 }
		};
	this->bind_(&addr, sizeof(addr));
	this->listen_();
};

void R::StreamServer::listen(const bool ip6, const int port) {
	if (ip6) {
		::sockaddr_in6 addr = {
				sin6_family: AF_INET6,
				sin6_port: htons(port),
				sin6_flowinfo: 0,
				sin6_addr: IN6ADDR_ANY_INIT,
				sin6_scope_id: 0
			};
		this->bind_(&addr, sizeof(addr));
	} else {
		::sockaddr_in addr = {
				sin_family: AF_INET,
				sin_port: htons(port),
				sin_addr: { s_addr: 0 }
			};
		this->bind_(&addr, sizeof(addr));
	}
	this->listen_();
};

void R::StreamServer::listen(const std::string& interface, const int port) {
	#warning Not implemented
};

void R::StreamServer::listen(const std::string& local) {
	#warning Not implemented
};

void R::StreamServer::bind_(void* addr, const size_t addrSize) {
	this->fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
	int v = ::fcntl(this->fd_, F_GETFL, 0);
	::fcntl(this->fd_, F_SETFL, v | O_NONBLOCK);
	v = 1;
	::setsockopt(this->fd_, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v));
	this->poller_ = this->pool_->add(this->fd_, onReadReady_, onWriteReady_, this);
};

void R::StreamServer::listen_() {
	::listen(this->fd_, SOMAXCONN);
};

void R::StreamServer::listen_(const int backlog) {
	::listen(this->fd_, backlog);
};

void R::StreamServer::clientDisconnected_(StreamClient& client) {
	auto it = this->clients_.find(client.fd_);
	if (it != this->clients_.end()) {
		for (auto cit = this->onClientDisconnect_.begin(); cit != this->onClientDisconnect_.end(); ++cit)
			(*cit)(*this, client);
		R::StreamClient* client = it->second;
		this->clients_.erase(it);
		delete client;
	}
};

bool R::StreamServer::onReadReady_(Poller& poller, int fd, void* arg) {
	StreamServer* server = reinterpret_cast<StreamServer*>(arg);
	while (true) {
		int cliFD = ::accept(fd, NULL, NULL);
		if (cliFD == -1) {
			//#TODO: Test for actual error (okay: EINTR, EAGAIN, or EWOULDBLOCK)
			return true;
		}
		StreamClient* client = new StreamClient(*(server->pool_), *server, cliFD);
		server->clients_[cliFD] = client;
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


