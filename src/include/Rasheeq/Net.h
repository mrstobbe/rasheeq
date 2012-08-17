#ifndef Rasheeq_Net_H_
#define Rasheeq_Net_H_

#if __cplusplus

#include <string>

namespace R {
namespace Net {

enum Protocol {
	#if RASHEEQ_HAVE_SCTP
		protSCTP,
		protDGSCTP,
	#endif
	protTCP,
	//protUDG, //#TODO: Support emulated UDG-like functionality over Windows named pipes
	protUDP,
	protLocal
}; //enum Protocol

enum StreamProtocol {
	#if RASHEEQ_HAVE_SCTP
		spSCTP = protSCTP,
	#endif
	spTCP = protTCP,
	spLocal = protLocal
}; //enum StreamProtocol

enum DatagramProtocol {
	#if RASHEEQ_HAVE_SCTP
		dgpDGSCTP = protDGSCTP,
	#endif
	//#TODO: Support emulated UDG-like functionality over Windows named pipes
	//dgprotUDG = protUDG,
	dgpUDP = protUDP
}; //enum DatagramProtocol

enum AddressFamily {
	afIP4,
	afIP6,
	afLocal
}; //enum AddressFamily

class DatagramAddr {
	protected:
		void* addr_;
		size_t addrSize_;
		AddressFamily family_;
		DatagramProtocol protocol_;
	public:
		DatagramAddr();
		DatagramAddr(const std::string& address);
		DatagramAddr(const std::string& address, const int port);
		DatagramAddr(const int port);
		DatagramAddr(const DatagramAddr& copy);
		DatagramAddr(DatagramAddr&& move);
		~DatagramAddr();
		DatagramAddr& operator =(const DatagramAddr& move);
		DatagramAddr& operator =(DatagramAddr&& move);
		DatagramAddr& operator =(const std::string& address);
		operator size_t() const;
		operator void*() const;
	public:
		int nativeAddrFamily() const;
		int nativeDomain() const;
		int nativeProtocol() const;
		void parse(const std::string& address);
		void parse(const std::string& address, const int port);
		void parse(const int port);
		void reset();
}; //class DatagramAddr

class StreamAddr {
	protected:
		void* addr_;
		size_t addrSize_;
		AddressFamily family_;
		StreamProtocol protocol_;
	public:
		StreamAddr();
		StreamAddr(const std::string& address);
		StreamAddr(const std::string& address, const int port);
		StreamAddr(const int port);
		StreamAddr(const StreamAddr& copy);
		StreamAddr(StreamAddr&& move);
		~StreamAddr();
		StreamAddr& operator =(const StreamAddr& copy);
		StreamAddr& operator =(StreamAddr&& move);
		StreamAddr& operator =(const std::string& address);
		operator size_t() const;
		operator void*() const;
	public:
		int nativeAddrFamily() const;
		int nativeDomain() const;
		int nativeProtocol() const;
		void parse(const std::string& address);
		void parse(const std::string& address, const int port);
		void parse(const int port);
		void reset();
}; //class StreamAddr



}; //ns Net
}; //ns R

extern "C" {
#endif /* __cplusplus */


#if __cplusplus
}; //extern "C"
#endif /* __cplusplus */

#endif /* Rasheeq_Net_H_ */
