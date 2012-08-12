#ifndef Rasheeq_StreamClient_H_
#define Rasheeq_StreamClient_H_

#if __cplusplus

#include <list>
#include <string>

namespace R {

class StreamClient {
	friend class StreamServer;
	public:
		enum State {
			scsDisconnected,
			scsBound,
			scsConnecting,
			scsConnected,
			scsDisconnecting
		};
		typedef std::function<void(StreamClient& client)> Connected;
		typedef std::function<void(StreamClient& client)> Destructing;
		typedef std::function<void(StreamClient& client)> Disconnected;
		typedef std::function<void(StreamClient& client)> Disconnecting;
		typedef std::function<void(StreamClient& client, std::string& data, size_t& offset, size_t& size)> ReceivedData;
	public:
		static size_t bufferSize;
	protected:
		std::string inBuf_;
		int fd_;
		std::string outBuf_;
		Poller* poller_;
		StreamServer* server_;
		State state_;
		void* userData_;
	protected:
		std::list<Connected> onConnect_;
		std::list<Destructing> onDestruct_;
		std::list<Disconnected> onDisconnect_;
		std::list<ReceivedData> onReceivedData_;
	protected:
		StreamClient(PollerPool& pool, StreamServer& server, int fd);
	public:
		StreamClient(StreamClient&& move);
		~StreamClient();
	public:
		void disconnect();
		void send(const std::string& data);
	private:
		static bool onReadReady_(Poller& poller, int fd, void* arg);
		static bool onWriteReady_(Poller& poller, int fd, void* arg);
		static void onError_(Poller& poller, int fd, void* arg);
	private:
		StreamClient(const StreamClient& copy) { };
		StreamClient& operator =(const StreamClient& copy) { return *this; };
}; //class StreamClient

}; //ns R

extern "C" {
#endif /* __cplusplus */



#if __cplusplus
}; //extern "C"
#endif /* __cplusplus */

#endif /* Rasheeq_StreamClient_H_ */
