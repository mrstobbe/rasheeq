#include <Rasheeq.h>

#include <string>
#include <iostream>

using namespace std;

int main(int argc, char** argv) {
	R::PollerPool pool;
	R::Poller& poller = pool.createPoller(50);
	R::StreamClient socket(pool);
	socket.userData(new string());
	socket.onDestruct([](R::StreamClient& socket) {
		delete reinterpret_cast<string*>(socket.userData());
		cerr << "string destroyed" << endl;
	});
	socket.onConnect([](R::StreamClient& socket) {
		socket.send("GET / HTTP/1.1\r\nHost: www.google.com\r\nConnection: close\r\n\r\n");
	});
	socket.onReceivedData([](R::StreamClient& socket, std::string& buf, size_t& offs, size_t& size) {
		cout << buf << flush;
		buf.clear();
	});
	socket.connect("tcp://74.125.225.129:80");
	while (!socket.isDisconnecting()) {
		poller.poll();
	}
	socket.close();
};
