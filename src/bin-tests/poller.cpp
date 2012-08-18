#include <Rasheeq/Poller.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#include <cerrno>
#include <cstdio>
#include <iostream>

using namespace std;


static bool writeReady(R::Poller& poller, int sock, void* userArg);
static bool readReady(R::Poller& poller, int sock, void* userArg);
static void errorOccured(R::Poller& poller, int sock, void* userArg);

static bool readReady(R::Poller& poller, int sock, void* userArg) {
	cout << "readReady(" << sock << ")" << endl;
	int& listener = *reinterpret_cast<int*>(userArg);
	if (sock == listener) {
		sockaddr_in paddr;
		socklen_t paddr_len = sizeof(paddr);
		int cli = accept(listener, reinterpret_cast<sockaddr*>(&paddr), &paddr_len);
		cout << inet_ntoa(paddr.sin_addr) << " connected." << endl;
		int v = fcntl(cli, F_GETFL, 0);
		fcntl(cli, F_SETFL, v | O_NONBLOCK);
		poller.add(cli, [](R::Poller& sender, int fd, void* arg) { }, readReady, writeReady, errorOccured, userArg);
	} else {
		char buf[512];
		int res = recv(sock, buf, 512, 0);
		//Do something useful with the data here
		if ((res == -1) && ((errno == EWOULDBLOCK) || (errno == EAGAIN)))
			return false;
		if (res <= 0) {
			sockaddr_in paddr;
			socklen_t paddr_len = sizeof(paddr);
			getpeername(sock, reinterpret_cast<sockaddr*>(&paddr), &paddr_len);
			cout << inet_ntoa(paddr.sin_addr) << " disconnected." << endl;
			poller.remove(sock);
		}
	}
	return true;
};

static bool writeReady(R::Poller& poller, int sock, void* userArg) {
	cout << "writeReady(" << sock << ")" << endl;
	return true;
};

static void errorOccured(R::Poller& poller, int sock, void* userArg) {
	cout << "error(" << sock << ")" << endl;
	poller.remove(sock);
};

int main(int argc, char** argv) {
	R::Poller poller;
	int listener = ::socket(AF_INET, SOCK_STREAM, 0);
	if (listener == -1) {
		perror("socket()");
		return 1;
	}
	int v = ::fcntl(listener, F_GETFL, 0);
	if (v == -1) {
		perror("fcntl(get)");
		return 1;
	}
	if (::fcntl(listener, F_SETFL, v | O_NONBLOCK) == -1) {
		perror("fcntl(set)");
		return 1;
	}
	sockaddr_in addr = {
		sin_family: AF_INET,
		sin_port: htons(12345),
		sin_addr: { s_addr: 0 }
	};
	v = 1;
	if (::setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &v, sizeof(v)) == -1) {
		perror("setsockopt()");
		return 1;
	}
	if (::bind(listener, reinterpret_cast<const sockaddr*>(&addr), sizeof(addr)) == -1) {
		perror("bind()");
		return 1;
	}
	if (::listen(listener, SOMAXCONN) == -1) {
		perror("listen()");
		return 1;
	}
	poller.add(listener, [](R::Poller& poller, int fd, void* arg) { }, readReady, writeReady, &listener);
	while (true) {
		poller.poll();
	}
	return 0;
};
