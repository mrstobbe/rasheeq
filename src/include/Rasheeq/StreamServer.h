#ifndef Rasheeq_StreamServer_H_
#define Rasheeq_StreamServer_H_

#if __cplusplus

#include <string>
#include <list>
#include <unordered_map>
#include <functional>

namespace R {

class StreamServer {
	friend class StreamClient;
	public:
		typedef std::function<void(StreamServer& server, StreamClient& client)> ClientAccepted;
		typedef std::function<bool(StreamServer& server, StreamClient& client)> ClientConnected;
		typedef std::function<void(StreamServer& server, StreamClient& client)> ClientDisconnected;
		typedef std::function<void(StreamServer& server)> Destructing;
	protected:
		std::unordered_map<int, StreamClient*> clients_;
		int fd_;
		bool greedy_;
		Poller* poller_;
		PollerPool* pool_;
		void* userData_;
	protected:
		std::list<ClientConnected> onClientConnect_;
		std::list<ClientDisconnected> onClientDisconnect_;
		std::list<Destructing> onDestruct_;
	public:
		StreamServer(PollerPool& pool);
		StreamServer(StreamServer&& move);
		~StreamServer();
	public:
		void listen(const int port);
		void listen(const bool ip6, const int port);
		void listen(const std::string& interface, const int port);
		void listen(const std::string& local);
	private:
		void bind_(void* addr, const size_t addrSize);
		void listen_();
		void listen_(const int backlog);
		void clientDisconnected_(StreamClient& client);
	private:
		StreamServer(const StreamServer& copy) { };
		StreamServer& operator =(const StreamServer& copy) { return *this; };
	private:
		static bool onReadReady_(Poller& poller, int fd, void* arg);
		static bool onWriteReady_(Poller& poller, int fd, void* arg);
}; //class StreamServer

}; //ns R

extern "C" {
#endif /* __cplusplus */



#if __cplusplus
}; //extern "C"
#endif /* __cplusplus */

#endif /* Rasheeq_StreamServer_H_ */
