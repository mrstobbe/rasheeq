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

size_t R::StreamClient::ioBufferSize = 0x1000;

R::StreamClient::StreamClient(PollerPool& pool, StreamServer& server, int fd):
	fd_(fd),
	inBuf_(),
	localAddr_(),
	outBuf_(),
	poller_(NULL),
	pool_(&pool),
	remoteAddr_(),
	server_(&server),
	state_(scsConnected),
	userData_(NULL),
	onConnect_(),
	onDestruct_(),
	onDisconnect_(),
	onDisconnecting_(),
	onReady_(),
	onReceivedData_(),
	onWriteDataReady_()
{
	::sockaddr_storage sa;
	::socklen_t sal = sizeof(::sockaddr_storage);
	if (::getsockname(this->fd_, reinterpret_cast< ::sockaddr* >(&sa), &sal) == -1) {
		//#TODO: Exception
	} else {
		this->localAddr_ = Net::StreamAddr(this->server_->listenAddr_.protocol(), &sa);
	}
	if (::getpeername(this->fd_, reinterpret_cast< ::sockaddr* >(&sa), &sal) == -1) {
		//#TODO: Exception
	} else {
		this->remoteAddr_ = Net::StreamAddr(this->server_->listenAddr_.protocol(), &sa);
	}
	int v = ::fcntl(this->fd_, F_GETFL, 0);
	::fcntl(this->fd_, F_SETFL, v | O_NONBLOCK);
	this->poller_ = &pool.add(this->fd_, onAdded_, onReadReady_, onWriteReady_, onError_, this);
};

R::StreamClient::StreamClient(PollerPool& pool):
	fd_(),
	inBuf_(),
	localAddr_(),
	outBuf_(),
	poller_(NULL),
	pool_(&pool),
	remoteAddr_(),
	server_(NULL),
	state_(scsConnected),
	userData_(NULL),
	onConnect_(),
	onDestruct_(),
	onDisconnect_(),
	onDisconnecting_(),
	onReady_(),
	onReceivedData_(),
	onWriteDataReady_()
{
};

R::StreamClient::StreamClient(StreamClient&& move):
	fd_(std::move(move.fd_)),
	inBuf_(std::move(move.inBuf_)),
	localAddr_(std::move(move.localAddr_)),
	outBuf_(std::move(move.outBuf_)),
	poller_(std::move(move.poller_)),
	pool_(std::move(move.pool_)),
	remoteAddr_(std::move(move.remoteAddr_)),
	server_(std::move(move.server_)),
	state_(std::move(move.state_)),
	userData_(std::move(move.userData_)),
	onConnect_(std::move(move.onConnect_)),
	onDestruct_(std::move(move.onDestruct_)),
	onDisconnect_(std::move(move.onDisconnect_)),
	onDisconnecting_(std::move(move.onDisconnecting_)),
	onReady_(std::move(move.onReady_)),
	onReceivedData_(std::move(move.onReceivedData_)),
	onWriteDataReady_(std::move(move.onWriteDataReady_))
{
	move.fd_ = -1;
	move.poller_ = NULL;
	move.pool_ = NULL;
	move.server_ = NULL;
};

R::StreamClient::~StreamClient() {
	if (this->fd_ != -1) {
		this->poller_->remove(this->fd_);
		this->fd_ = -1;
		this->state_ = scsDisconnected;
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


int R::StreamClient::fd() const {
	return this->fd_;
};

R::Net::StreamAddr const& R::StreamClient::localAddr() const {
	return this->localAddr_;
};

bool R::StreamClient::isConnected() const {
	return ((this->state_ == scsConnected) && (this->state_ == scsDisconnecting));
};

bool R::StreamClient::isConnecting() const {
	return (this->state_ == scsConnecting);
};

bool R::StreamClient::isDisconnected() const {
	return (this->state_ == scsDisconnected);
};

bool R::StreamClient::isDisconnecting() const {
	return (this->state_ == scsDisconnecting);
};

R::Net::StreamAddr const& R::StreamClient::remoteAddr() const {
	return this->remoteAddr_;
};

R::StreamClient::State R::StreamClient::state() const {
	return this->state_;
};

void* R::StreamClient::userData() const {
	return this->userData_;
};

void R::StreamClient::userData(void* value) {
	this->userData_ = value;
};


#include <iostream>

void R::StreamClient::bind(const Net::StreamAddr& address) {
	//#TODO: State = scsBinding (or something)
	this->bind_(address);
};

void R::StreamClient::connect(const Net::StreamAddr& address) {
	if (this->state_ != scsReady) {
		//#TODO: Queue connection or exception
		return;
	}
	this->state_ = scsConnecting;
	this->remoteAddr_ = address;
	int res = ::connect(this->fd_, reinterpret_cast<const sockaddr*>((void*)address), (size_t)address);
	if ((res == -1) && (errno != EINPROGRESS)) {
		//#TODO: Throw exception
		return;
	}
	std::cout << "connect() -> " << res << std::endl;
	::sockaddr_storage sa;
	::socklen_t sal = sizeof(::sockaddr_storage);
	if (::getsockname(this->fd_, reinterpret_cast< ::sockaddr* >(&sa), &sal) == -1) {
		//#TODO: Exception
	} else {
		this->localAddr_ = Net::StreamAddr(address.protocol(), &sa);
	}
};

void R::StreamClient::close() {
	if (this->fd_ != -1) {
		::shutdown(this->fd_, SHUT_RDWR);
		this->poller_->remove(this->fd_);
		this->fd_ = -1;
		this->state_ = scsDisconnected;
		for (auto it = this->onDisconnect_.begin(); it != this->onDisconnect_.end(); ++it)
			(*it)(*this);
		if (this->server_ != NULL)
			this->server_->clientDisconnected_(*this);
		this->localAddr_.reset();
		this->remoteAddr_.reset();
	}
};

bool R::StreamClient::flush() {
	if (this->outBuf_.size() == 0)
		return true;
	return this->send(std::string());
};

void R::StreamClient::halfClose() {
	if (::shutdown(this->fd_, SHUT_WR) == -1) {
		//#TODO: Exception
		return;
	}
	this->state_ = scsDisconnecting;
};

bool R::StreamClient::send(const std::string& data) {
	this->outBuf_.append(data);
	size_t size = this->outBuf_.size();
	if (size > R::StreamClient::ioBufferSize)
		size = R::StreamClient::ioBufferSize;
	ssize_t res = ::send(this->fd_, this->outBuf_.data(), size, 0);
	if (res == -1) {
		if ((errno == EWOULDBLOCK) || (errno == EAGAIN) || (errno == EINTR)) {
		} else if (errno == ECONNRESET) {
			this->close();
		} else {
			//#TODO: Handle errors appropriately
			this->close();
		}
		return (this->outBuf_.size() == 0);
	}
	if (res != 0)
		this->outBuf_.erase(0, res);
	return (this->outBuf_.size() == 0);
};

void R::StreamClient::onConnect(const Connected& callback) {
	this->onConnect_.push_back(callback);
};

void R::StreamClient::onDestruct(const Destructing& callback) {
	this->onDestruct_.push_back(callback);
};

void R::StreamClient::onDisconnecting(const Disconnecting& callback) {
	this->onDisconnecting_.push_back(callback);
};

void R::StreamClient::onDisconnect(const Disconnected& callback) {
	this->onDisconnect_.push_back(callback);
};

void R::StreamClient::onReady(const Ready& callback) {
	this->onReady_.push_back(callback);
};

void R::StreamClient::onReceivedData(const ReceivedData& callback) {
	this->onReceivedData_.push_back(callback);
};

void R::StreamClient::onWriteDataReady(const WriteDataReady& callback) {
	this->onWriteDataReady_.push_back(callback);
};


#include <cstdio>

void R::StreamClient::bind_(const Net::StreamAddr& address) {
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

	/*
	StreamAddress localAddr = address.localEquiv();
	if (::bind(this->fd_, reinterpret_cast<const sockaddr*>((void*)localAddr), (size_t)localAddr) == -1) {
		perror("bind()");
		return;
	}
	*/
	if (::bind(this->fd_, reinterpret_cast<const sockaddr*>((void*)address), (size_t)address) == -1) {
		perror("bind()");
		return;
	}
	this->poller_ = &this->pool_->add(this->fd_, onAdded_, onReadReady_, onWriteReady_, this);
};


void R::StreamClient::onAdded_(Poller& poller, int fd, void* arg) {
	StreamClient* client = reinterpret_cast<StreamClient*>(arg);
	client->state_ = scsReady;
	for (auto it = client->onReady_.begin(); it != client->onReady_.end(); ++it)
		(*it)(*client);
};

bool R::StreamClient::onReadReady_(Poller& poller, int fd, void* arg) {
	StreamClient* client = reinterpret_cast<StreamClient*>(arg);
	char buf[R::StreamClient::ioBufferSize];
	ssize_t res = ::recv(client->fd_, buf, R::StreamClient::ioBufferSize, 0);
	if (res == -1) {
		if ((errno == EWOULDBLOCK) || (errno == EAGAIN) || (errno == EINTR)) {
			return true;
		} else if (errno == ECONNRESET) {
			client->close();
		} else {
			//#TODO: Handle errors appropriately
			client->close();
		}
		return true;
	} else if (res == 0) {
		client->state_ = scsDisconnecting;
		for (auto it = client->onDisconnecting_.begin(); it != client->onDisconnecting_.end(); ++it)
			(*it)(*client);
		return true;
	} else {
		size_t offs = client->inBuf_.size();
		size_t size = res;
		client->inBuf_.append(buf, res);
		for (auto it = client->onReceivedData_.begin(); it != client->onReceivedData_.end(); ++it)
			(*it)(*client, client->inBuf_, offs, size);
		return false;
	}
};

bool R::StreamClient::onWriteReady_(Poller& poller, int fd, void* arg) {
	StreamClient* client = reinterpret_cast<StreamClient*>(arg);
	if (client->state_ == scsConnecting) {
		client->state_ = scsConnected;
		::sockaddr_storage sa;
		::socklen_t sal = sizeof(::sockaddr_storage);
		if (::getsockname(client->fd_, reinterpret_cast< ::sockaddr* >(&sa), &sal) == -1) {
			//#TODO: Exception
		} else {
			client->localAddr_ = Net::StreamAddr(client->localAddr_.protocol(), &sa);
		}
		if (::getpeername(client->fd_, reinterpret_cast< ::sockaddr* >(&sa), &sal) == -1) {
			//#TODO: Exception
		} else {
			client->remoteAddr_ = Net::StreamAddr(client->localAddr_.protocol(), &sa);
		}
		for (auto it = client->onConnect_.begin(); it != client->onConnect_.end(); ++it)
			(*it)(*client);
	}
	for (auto it = client->onWriteDataReady_.begin(); it != client->onWriteDataReady_.end(); ++it)
		(*it)(*client, client->outBuf_);

	if (client->outBuf_.size() == 0)
		return true;

	size_t size = client->outBuf_.size();
	if (size > R::StreamClient::ioBufferSize)
		size = R::StreamClient::ioBufferSize;
	ssize_t res = ::send(client->fd_, client->outBuf_.data(), size, 0);
	if (res == -1) {
		if ((errno == EWOULDBLOCK) || (errno == EAGAIN) || (errno == EINTR)) {
			return true;
		} else if (errno == ECONNRESET) {
			client->close();
		} else {
			//#TODO: Handle errors appropriately
			client->close();
		}
		return true;
	}
	if (res != 0)
		client->outBuf_.erase(0, res);
	return (client->outBuf_.size() == 0);
};

void R::StreamClient::onError_(Poller& poller, int fd, void* arg) {
	StreamClient* client = reinterpret_cast<StreamClient*>(arg);
	//#TODO: Essentially stubbed
	client->close();
};

