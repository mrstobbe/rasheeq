
#include <Rasheeq/Poller.h>

#include <string>
#include <iostream>

namespace R {

class TCPServer {
	protected:
		int fd_;
		bool greedy_;
		Poller* poller_;
	public:
		TCPServer(Poller* poller);
		TCPServer(TCPServer&& move);
		~TCPServer();
		TCPServer& operator =(TCPServer&& move);
	public:
		void listen(const int port);
		void listen(const bool ip6, const int port);
		void listen(const std::string& ip, const int port);
		void listen(const std::string& local);
	private:
		void bind_(void* addr, const size_t addrSize);
		void listen_();
		void listen_(const int backlog);
	private:
		TCPServer(const TCPServer& copy) { };
		TCPServer& operator =(const TCPServer& copy) { return *this; };
	private:
		static bool onReadReady(int fd, TCPServer* server);
		static bool onWriteReady(int fd, TCPServer* server);
}; //class TCPServer

}; //ns R

int main(int argc, char** argv) {
	R::Poller poller;
	R::TCPServer server(&poller);
};



#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

R::TCPServer::TCPServer(R::Poller* poller):
	fd_(-1),
	greedy_(false),
	poller_(poller)
{
	//#TODO: This should be lazy-init so that the AF can be determined via what's passed to listen()
	/*
	*/
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

void R::TCPServer::listen(const std::string& ip, const int port) {
	#warning Not implemented
};

void R::TCPServer::listen(const std::string& local) {
	#warning Not implemented
};

void R::TCPServer::bind_(void* addr, const size_t addrSize) {
	this->fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
	#warning Obviously those lambdas one line down are just stubs
	this->poller_->add(this->fd_, [](int fd, void*)->bool { return true; }, [](int fd, void*)->bool { return true; });
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

bool R::TCPServer::onReadReady(int fd, TCPServer* server) {
	int cli = ::accept(fd, NULL, NULL);
	if (cli == -1)
		return true;
	//#TODO: Do something with the client
	//#TODO: Support greedy (loop until would block)
	#warning Stubbed. Obviously this should not close
	::close(cli);
	return false;
};

bool R::TCPServer::onWriteReady(int fd, TCPServer* server) {
	return true;
};

