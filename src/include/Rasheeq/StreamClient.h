#ifndef Rasheeq_StreamClient_H_
#define Rasheeq_StreamClient_H_

#if __cplusplus

#include <list>

namespace R {

class StreamClient {
	friend class StreamServer;
	public:
		typedef std::function<void(StreamClient& client)> Connected;
		typedef std::function<void(StreamClient& client)> Destructing;
		typedef std::function<void(StreamClient& client)> Disconnected;
	protected:
		int fd_;
		Poller* poller_;
		StreamServer* server_;
		void* userData_;
	protected:
		std::list<Connected> onConnect_;
		std::list<Destructing> onDestruct_;
		std::list<Disconnected> onDisconnect_;
	protected:
		StreamClient(PollerPool& pool, StreamServer& server, int fd);
	public:
		StreamClient(StreamClient&& move);
		~StreamClient();
	public:
		void disconnect();
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
