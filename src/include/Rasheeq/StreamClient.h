#ifndef Rasheeq_StreamClient_H_
#define Rasheeq_StreamClient_H_

#if __cplusplus

#include <list>
#include <string>
#include <functional>

namespace R {

class StreamClient {
	friend class StreamServer;
	public:
		enum State {
			scsDisconnected,
			scsReady,
			scsConnecting,
			scsConnected,
			scsDisconnecting
		};
		typedef std::function<void(StreamClient& client)> Connected;
		typedef std::function<void(StreamClient& client)> Destructing;
		typedef std::function<void(StreamClient& client)> Disconnected;
		typedef std::function<void(StreamClient& client)> Disconnecting;
		typedef std::function<void(StreamClient& client, std::string& data, size_t& offset, size_t& size)> ReceivedData;
		typedef std::function<void(StreamClient& client)> Ready;
		typedef std::function<void(StreamClient& client, std::string& outBuffer)> WriteDataReady;
	public:
		static size_t ioBufferSize;
	protected:
		int fd_;
		std::string inBuf_;
		Net::StreamAddr localAddr_;
		std::string outBuf_;
		Poller* poller_;
		PollerPool* pool_;
		Net::StreamAddr remoteAddr_;
		StreamServer* server_;
		State state_;
		void* userData_;
	protected:
		std::list<Connected> onConnect_;
		std::list<Destructing> onDestruct_;
		std::list<Disconnected> onDisconnect_;
		std::list<Disconnecting> onDisconnecting_;
		std::list<Ready> onReady_;
		std::list<ReceivedData> onReceivedData_;
		std::list<WriteDataReady> onWriteDataReady_;
	protected:
		StreamClient(PollerPool& pool, StreamServer& server, int fd);
	public:
		StreamClient(PollerPool& pool);
		StreamClient(StreamClient&& move);
		~StreamClient();
	public:
		int fd() const;
		Net::StreamAddr const& localAddr() const;
		bool isConnected() const;
		bool isConnecting() const;
		bool isDisconnected() const;
		bool isDisconnecting() const;
		Net::StreamAddr const& remoteAddr() const;
		State state() const;
		void* userData() const;
		void userData(void* value);
	public:
		void bind(const Net::StreamAddr& address);
		template<typename...Args>
		void bind(Args...args) { this->bind(Net::StreamAddr(args...)); };
		void connect(const Net::StreamAddr& address);
		template<typename...Args>
		void connect(Args...args) { this->connect(Net::StreamAddr(args...)); };
		void close();
		bool flush();
		void halfClose();
		bool send(const std::string& data);
	public:
		void onConnect(const Connected& callback);
		void onDestruct(const Destructing& callback);
		void onDisconnecting(const Disconnecting& callback);
		void onDisconnect(const Disconnected& callback);
		void onReady(const Ready& callback);
		void onReceivedData(const ReceivedData& callback);
		void onWriteDataReady(const WriteDataReady& callback);
	protected:
		void bind_(const Net::StreamAddr& address);
	private:
		static void onAdded_(Poller& poller, int fd, void* arg);
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
