
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
			cout << "{" << (string)client.remoteAddr() << "} connected" << endl;
			R::StreamClient* proxy = new R::StreamClient(server.pollerPool());
			client.userData(proxy);
			proxy->userData(&client);
			client.onReceivedData([](R::StreamClient& client, std::string& buf, size_t& offs, size_t& size) {
					cout << "client:{" << (string)client.remoteAddr() << "(" << client.fd() << ") <-> " << (string)reinterpret_cast<R::StreamClient*>(client.userData())->localAddr() << "(" << reinterpret_cast<R::StreamClient*>(client.userData())->fd() << ")} inbound(size=" << size << ")" << endl;
					reinterpret_cast<R::StreamClient*>(client.userData())->send(buf);
					buf.clear();
				});
			client.onWriteDataReady([](R::StreamClient& client, std::string& buf) {
					cout << "client:{" << (string)client.remoteAddr() << "(" << client.fd() << ") <-> " << (string)reinterpret_cast<R::StreamClient*>(client.userData())->localAddr() << "(" << reinterpret_cast<R::StreamClient*>(client.userData())->fd() << ")} write-ready(size=" << buf.size() << ")" << endl;
					if ((client.isDisconnecting()) && (client.flush()))
						client.close();
				});
			client.onDisconnecting([](R::StreamClient& client) {
					cout << "client:{" << (string)client.remoteAddr() << "(" << client.fd() << ") <-> " << (string)reinterpret_cast<R::StreamClient*>(client.userData())->localAddr() << "(" << reinterpret_cast<R::StreamClient*>(client.userData())->fd() << ")} half-close" << endl;
					reinterpret_cast<R::StreamClient*>(client.userData())->halfClose();
					if (client.flush())
						client.close();
				});
			client.onDisconnect([](R::StreamClient& client) {
					cout << "client:{" << (string)client.remoteAddr() << "(" << client.fd() << ") <-> " << (string)reinterpret_cast<R::StreamClient*>(client.userData())->localAddr() << "(" << reinterpret_cast<R::StreamClient*>(client.userData())->fd() << ")} disconnect" << endl;
					client.close();
				});
			client.onDestruct([](R::StreamClient& client) {
					delete reinterpret_cast<R::StreamClient*>(client.userData());
				});

			proxy->onReady([](R::StreamClient& proxy) {
					cout << "server:{" << (string)reinterpret_cast<R::StreamClient*>(proxy.userData())->remoteAddr() << "(" << reinterpret_cast<R::StreamClient*>(proxy.userData())->fd() << ") <-> " << (string)proxy.localAddr() << "(" << proxy.fd() << ")} bound" << endl;
					proxy.connect("tcp://127.0.0.1:80");
				});
			proxy->onConnect([](R::StreamClient& proxy) {
					cout << "server:{" << (string)reinterpret_cast<R::StreamClient*>(proxy.userData())->remoteAddr() << "(" << reinterpret_cast<R::StreamClient*>(proxy.userData())->fd() << ") <-> " << (string)proxy.localAddr() << "(" << proxy.fd() << ")} connect" << endl;
				});
			proxy->onReceivedData([](R::StreamClient& proxy, std::string& buf, size_t& offs, size_t& size) {
					cout << "server:{" << (string)reinterpret_cast<R::StreamClient*>(proxy.userData())->remoteAddr() << "(" << reinterpret_cast<R::StreamClient*>(proxy.userData())->fd() << ") <-> " << (string)proxy.localAddr() << "(" << proxy.fd() << ")} inbound(size=" << size << ")" << endl;
					reinterpret_cast<R::StreamClient*>(proxy.userData())->send(buf);
					buf.clear();
				});
			proxy->onWriteDataReady([](R::StreamClient& proxy, std::string& buf) {
					cout << "server:{" << (string)reinterpret_cast<R::StreamClient*>(proxy.userData())->remoteAddr() << "(" << reinterpret_cast<R::StreamClient*>(proxy.userData())->fd() << ") <-> " << (string)proxy.localAddr() << "(" << proxy.fd() << ")} write-ready(size=" << buf.size() << ")" << endl;
					if ((proxy.isDisconnecting()) && (proxy.flush()))
						proxy.close();
				});
			proxy->onDisconnecting([](R::StreamClient& proxy) {
					cout << "server:{" << (string)reinterpret_cast<R::StreamClient*>(proxy.userData())->remoteAddr() << "(" << reinterpret_cast<R::StreamClient*>(proxy.userData())->fd() << ") <-> " << (string)proxy.localAddr() << "(" << proxy.fd() << ")} half-close" << endl;
					reinterpret_cast<R::StreamClient*>(proxy.userData())->halfClose();
					if (proxy.flush())
						proxy.close();
				});
			proxy->onDisconnect([](R::StreamClient& proxy) {
					cout << "server:{" << (string)reinterpret_cast<R::StreamClient*>(proxy.userData())->remoteAddr() << "(" << reinterpret_cast<R::StreamClient*>(proxy.userData())->fd() << ") <-> " << (string)proxy.localAddr() << "(" << proxy.fd() << ")} disconnect" << endl;
					proxy.close();
				});
			proxy->bind("tcp://*");
		});
	server.onClientDisconnect([](R::StreamServer& server, R::StreamClient& client) {
			cout << "{" << (string)client.remoteAddr() << "} disconnected" << endl;
		});
	server.onDestruct([](R::StreamServer& server) {
			cout << "Server shutdown" << endl;
		});
	server.listen("tcp://[*]:12345");
	cout << "Server listening on " << (string)server.listenAddr() << endl;
	::signal(SIGINT, sigHandler);
	::signal(SIGTERM, sigHandler);
	while (okay) {
		poller.poll();
	}
};
