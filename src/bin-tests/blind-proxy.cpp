
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
		void bind_(fatally stubbed);
		void listen_(const int backlog);
	private:
		TCPServer(const TCPServer& copy) { };
		TCPServer& operator =(const TCPServer& copy) { return *this; };
}; //class TCPServer

}; //ns R

int main(int argc, char** argv) {
	R::Poller poller;
	R::TCPServer server(&poller);
};



#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

R::TCPServer::TCPServer(R::Poller* poller):
	fd_(-1),
	greedy_(false),
	poller_(poller)
{
	//#TODO: This should be lazy-init so that the AF can be determined via what's passed to listen()
	/*
	this->fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
	//Obviously those lambdas are just stubs
	poller->add(this->fd_, [](int fd, void*)->bool { return true; }, [](int fd, void*)->bool { return true; });
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
			sin_af: AF_INET,
			sin_port: ::htons(port),
			sin_addr: { s_addr: 0 }
		};
	this->bind_(reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
	this->listen_();
};

void R::TCPServer::listen(const bool ip6, const int port) {
	#error Left off here. This concept will fatal as it stands (baddr will deinit after if block)
	//#TODO: Rewrite
	sockaddr* addr = NULL;
	if (ip6) {
		::sockaddr_in baddr = {
				sin_af: AF_INET,
				sin_port: ::htons(port),
				sin_addr: { s_addr: 0 }
			};
	} else {
		::sockaddr_in6 baddr = {
				sin6_af: AF_INET6,
				sin6_port: ::htons(port),
				sin6_addr: IN6ADDR_ANY_INIT,
				sin6_flowinfo: { 0 },
				sin6_scope_id: { 0 }
			};
	}
	this->bind_(reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
	this->listen_();
};

void R::TCPServer::listen(const std::string& ip, const int port) {
	this->bind_(reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
	this->listen_();
};

void R::TCPServer::listen(const std::string& local) {
	this->bind_(reinterpret_cast<const sockaddr*>(&addr), sizeof(addr));
	this->listen_();
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

