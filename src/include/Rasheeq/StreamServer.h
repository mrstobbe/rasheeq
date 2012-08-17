#ifndef Rasheeq_StreamServer_H_
#define Rasheeq_StreamServer_H_

#if __cplusplus

#include <string>
#include <list>
#include <unordered_set>
#include <functional>

namespace R {

class StreamServer {
	friend class StreamClient;
	public:
		typedef std::function<void(StreamServer& server, StreamClient& client)> ClientAccepted;
		typedef std::function<void(StreamServer& server, StreamClient& client)> ClientConnected;
		typedef std::function<void(StreamServer& server, StreamClient& client)> ClientDisconnected;
		typedef std::function<void(StreamServer& server)> Destructing;
	protected:
		std::unordered_set<StreamClient*> clients_;
		int fd_;
		bool greedy_;
		Net::StreamAddr listenAddr_;
		Poller* poller_;
		PollerPool* pool_;
		std::unordered_set<StreamClient*> reapReady_;
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
		void listen(const Net::StreamAddr& address);
		void listen(const Net::StreamAddr& address, const int backlog);
		template<typename...Args>
		void listen(Args...args) { this->listen(Net::StreamAddr(args...)); };
	public:
		void onClientConnect(const ClientConnected& callback);
		void onClientDisconnect(const ClientDisconnected& callback);
		void onDestruct(const Destructing& callback);
	private:
		void bind_(const Net::StreamAddr& address);
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
