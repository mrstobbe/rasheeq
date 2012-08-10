
#include <Rasheeq/Poller.h>

#include <string>
#include <iostream>

#include <unordered_set>
namespace R {

class PollerPool {
	protected:
		std::unordered_set<Poller*> pollers_;
	public:
		PollerPool();
		~PollerPool();
	public:
		Poller* add(int fd, Poller::ReadReady onReadReady, Poller::WriteReady onWriteReady);
		Poller* add(int fd, Poller::ReadReady onReadReady, Poller::WriteReady onWriteReady, Poller::ErrorOccurred onError);
		Poller* add(int fd, Poller::ReadReady onReadReady, Poller::WriteReady onWriteReady, void* userArg);
		Poller* add(int fd, Poller::ReadReady onReadReady, Poller::WriteReady onWriteReady, Poller::ErrorOccurred onError, void* userArg);
		Poller* createPoller();
		bool remove(int fd);
	private:
		PollerPool(const PollerPool& copy) { };
		PollerPool& operator =(const PollerPool& copy) { return *this; };
};

}; //ns R

namespace R {

class TCPServer {
	protected:
		int fd_;
		bool greedy_;
		PollerPool* poller_;
	public:
		TCPServer(PollerPool* poller);
		TCPServer(TCPServer&& move);
		~TCPServer();
		TCPServer& operator =(TCPServer&& move);
	public:
		void listen(const int port);
		void listen(const bool ip6, const int port);
		void listen(const std::string& interface, const int port);
		void listen(const std::string& local);
	private:
		void bind_(void* addr, const size_t addrSize);
		void listen_();
		void listen_(const int backlog);
	private:
		TCPServer(const TCPServer& copy) { };
		TCPServer& operator =(const TCPServer& copy) { return *this; };
	private:
		static bool onReadReady(int fd, void* arg);
		static bool onWriteReady(int fd, void* arg);
}; //class TCPServer

class TCPClient {
	protected:
		TCPServer* server_;
	protected:
		TCPClient(TCPServer* server, int fd);
	public:
		TCPClient(TCPClient&& move);
		~TCPClient();
		TCPClient& operator =(TCPClient&& move);
}; //class TCPClient


}; //ns R

int main(int argc, char** argv) {
	R::PollerPool pool;
	pool->createPoller();
	R::TCPServer server(&pool);
};



#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

R::TCPServer::TCPServer(R::PollerPool* poller):
	fd_(-1),
	greedy_(false),
	poller_(poller)
{
};

R::TCPServer::TCPServer(TCPServer&& move):
	fd_(move.fd_),
	greedy_(move.greedy_),
	poller_(move.poller_)
{
	move.fd_ = -1;
	move.poller_ = NULL;
};

R::TCPServer::~TCPServer() {
	if (this->fd_ != -1) {
		this->poller_->remove(this->fd_);
		this->fd_ = -1;
	}
};

void R::TCPServer::listen(const int port) {
	::sockaddr_in addr = {
			sin_family: AF_INET,
			sin_port: htons(port),
			sin_addr: { s_addr: 0 }
		};
	this->bind_(&addr, sizeof(addr));
	this->listen_();
};

void R::TCPServer::listen(const bool ip6, const int port) {
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

void R::TCPServer::listen(const std::string& interface, const int port) {
	#warning Not implemented
};

void R::TCPServer::listen(const std::string& local) {
	#warning Not implemented
};

void R::TCPServer::bind_(void* addr, const size_t addrSize) {
	this->fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
	this->poller_->add(this->fd_, onReadReady, onWriteReady, this);
	int v = ::fcntl(this->fd_, F_GETFL, 0);
	::fcntl(this->fd_, F_SETFL, v | O_NONBLOCK);
	v = 1;
	::setsockopt(this->fd_, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v));
};

void R::TCPServer::listen_() {
	::listen(this->fd_, SOMAXCONN);
};

void R::TCPServer::listen_(const int backlog) {
	::listen(this->fd_, backlog);
};

R::TCPServer& R::TCPServer::operator =(TCPServer&& move) {
	if (this != &move) {
		if (this->fd_ != -1)
			this->poller_->remove(this->fd_);
		this->fd_ = move.fd_;
		this->poller_ = move.poller_;
		move.fd_ = -1;
		move.poller_ = NULL;
	}
	return *this;
};

bool R::TCPServer::onReadReady(int fd, void* arg) {
	TCPServer* server = reinterpret_cast<TCPServer*>(arg);
	while (true) {
		int cliFD = ::accept(fd, NULL, NULL);
		if (cliFD == -1) {
			//#TODO: Test for actual error (okay: EINTR, EAGAIN, or EWOULDBLOCK)
			return true;
		}
		int v = ::fcntl(cliFD, F_GETFL, 0);
		::fcntl(cliFD, F_SETFL, v | O_NONBLOCK);

		if (!sever->greedy_)
			break;
	}
	return false;
};

bool R::TCPServer::onWriteReady(int fd, void* arg) {
	return true;
};



R::TCPServer::TCPClient(TCPServer* server, int fd):
	fd_(fd),
	server_(server)
{
};

R::TCPServer::TCPClient(TCPClient&& move):
	fd_(move.fd_),
	server_(move.server_)
{
	move.fd_ = -1;
	move.server_ = NULL;
};

R::TCPServer::~TCPClient() {
	if (this->fd_ != -1) {
	}
};

R::TCPClient& R::TCPServer::operator =(TCPClient&& move) {
	return *this;
};

