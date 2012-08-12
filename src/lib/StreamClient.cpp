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

size_t R::StreamClient::bufferSize = 0x1000;

R::StreamClient::StreamClient(PollerPool& pool, StreamServer& server, int fd):
	inBuf_(),
	fd_(fd),
	outBuf_(),
	poller_(NULL),
	server_(&server),
	state_(scsConnected),
	userData_(NULL),
	onConnect_(),
	onDestruct_(),
	onDisconnect_(),
	onDisconnecting_(),
	onReceivedData_(),
	onWriteDataReady_()
{
	int v = ::fcntl(this->fd_, F_GETFL, 0);
	::fcntl(this->fd_, F_SETFL, v | O_NONBLOCK);
	this->poller_ = pool.add(this->fd_, onReadReady_, onWriteReady_, onError_, this);
};

R::StreamClient::StreamClient(StreamClient&& move):
	inBuf_(std::move(move.inBuf_)),
	fd_(move.fd_),
	outBuf_(std::move(move.outBuf_)),
	server_(move.server_),
	state_(move.state_),
	userData_(move.userData_),
	onConnect_(std::move(move.onConnect_)),
	onDestruct_(std::move(move.onDestruct_)),
	onDisconnect_(std::move(move.onDisconnect_)),
	onDisconnecting_(std::move(move.onDisconnecting_)),
	onReceivedData_(std::move(move.onReceivedData_)),
	onWriteDataReady_(std::move(move.onWriteDataReady_))
{
	move.fd_ = -1;
	move.poller_ = NULL;
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

R::StreamClient::State R::StreamClient::state() const {
	return this->state_;
};

void* R::StreamClient::userData() const {
	return this->userData_;
};

void R::StreamClient::userData(void* value) {
	this->userData_ = value;
};


bool R::StreamClient::flush() {
	if (this->outBuf_.size() == 0)
		return true;
	return this->send(std::string());
};

void R::StreamClient::disconnect() {
	if (this->fd_ != -1) {
		::shutdown(this->fd_, SHUT_RDWR);
		this->poller_->remove(this->fd_);
		this->fd_ = -1;
		this->state_ = scsDisconnected;
		for (auto it = this->onDisconnect_.begin(); it != this->onDisconnect_.end(); ++it)
			(*it)(*this);
		if (this->server_ != NULL)
			this->server_->clientDisconnected_(*this);
	}
};

bool R::StreamClient::send(const std::string& data) {
	this->outBuf_.append(data);
	size_t size = this->outBuf_.size();
	if (size > R::StreamClient::bufferSize)
		size = R::StreamClient::bufferSize;
	ssize_t res = ::send(this->fd_, this->outBuf_.data(), size, 0);
	if (res == -1) {
		if ((errno == EWOULDBLOCK) || (errno == EAGAIN) || (errno == EINTR)) {
		} else if (errno == ECONNRESET) {
			this->disconnect();
		} else {
			//#TODO: Handle errors appropriately
			this->disconnect();
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

void R::StreamClient::onReceivedData(const ReceivedData& callback) {
	this->onReceivedData_.push_back(callback);
};

void R::StreamClient::onWriteDataReady(const WriteDataReady& callback) {
	this->onWriteDataReady_.push_back(callback);
};

bool R::StreamClient::onReadReady_(Poller& poller, int fd, void* arg) {
	StreamClient* client = reinterpret_cast<StreamClient*>(arg);
	char buf[R::StreamClient::bufferSize];
	ssize_t res = ::recv(client->fd_, buf, R::StreamClient::bufferSize, 0);
	if (res == -1) {
		if ((errno == EWOULDBLOCK) || (errno == EAGAIN) || (errno == EINTR)) {
			return true;
		} else if (errno == ECONNRESET) {
			client->disconnect();
		} else {
			//#TODO: Handle errors appropriately
			client->disconnect();
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
		for (auto it = client->onConnect_.begin(); it != client->onConnect_.end(); ++it)
			(*it)(*client);
	}
	for (auto it = client->onWriteDataReady_.begin(); it != client->onWriteDataReady_.end(); ++it)
		(*it)(*client, client->outBuf_);

	if (client->outBuf_.size() == 0)
		return true;

	size_t size = client->outBuf_.size();
	if (size > R::StreamClient::bufferSize)
		size = R::StreamClient::bufferSize;
	ssize_t res = ::send(client->fd_, client->outBuf_.data(), size, 0);
	if (res == -1) {
		if ((errno == EWOULDBLOCK) || (errno == EAGAIN) || (errno == EINTR)) {
			return true;
		} else if (errno == ECONNRESET) {
			client->disconnect();
		} else {
			//#TODO: Handle errors appropriately
			client->disconnect();
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
	client->disconnect();
};

