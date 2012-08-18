
#include <Rasheeq.h>

#include <string>
#include <iostream>

#include <csignal>

using namespace std;

static bool okay = true;
static void sigHandler(int signo) { okay = false; };

int main(int argc, char** argv) {
	R::PollerPool pool;
	R::Poller& poller = pool.createPoller(50);
	R::StreamServer server(pool);
	server.onClientConnect([](R::StreamServer& server, R::StreamClient& client) {
			cout << "Client connected" << endl;
			client.onReceivedData([](R::StreamClient& client, std::string& buf, size_t& offs, size_t& size) {
					cout << "received: '" << buf << "'" << endl;
					buf.clear();
				});

			client.onWriteDataReady([](R::StreamClient& client, std::string& buf) {
					if ((client.isDisconnecting()) && (client.flush()))
						client.disconnect();
				});
	
			client.onDisconnecting([](R::StreamClient& client) {
					cout << "tcp half-close" << endl;
					if (client.flush())
						client.disconnect();
				});
			client.onDisconnect([](R::StreamClient& client) {
					cout << "Disconnected" << endl;
					client.disconnect();
				});
			client.onDestruct([](R::StreamClient& client) {
					cout << "Destructing" << endl;
				});
		});
	server.onClientDisconnect([](R::StreamServer& server, R::StreamClient& client) {
			cout << "Client disconnected" << endl;
		});
	server.onDestruct([](R::StreamServer& server) {
			cout << "Server shutdown" << endl;
		});
	server.listen("tcp://[*]:12345");
	::signal(SIGINT, sigHandler);
	::signal(SIGTERM, sigHandler);
	while (okay) {
		poller.poll();
	}
};
