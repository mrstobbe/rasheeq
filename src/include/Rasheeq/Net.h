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
	//dgprotUDG = protUDG, //#TODO: Support emulated UDG-like functionality over Windows named pipes
	dgpUDP = protUDP
}; //enum DatagramProtocol

enum AddressFamily {
	afIP4,
	afIP6,
	afLocal
}; //enum AddressFamily

class DatagramAddr {
	protected:
		AddressFamily family_;
		DatagramProtocol protocol_;
	public:
		DatagramAddr();
		DatagramAddr(const std::string& address);
		DatagramAddr(const std::string& address, const int port);
		~DatagramAddr();
		DatagramAddr& operator =(const std::string& address);
}; //class DatagramAddr

class StreamAddr {
	protected:
		AddressFamily family_;
		StreamProtocol protocol_;
	public:
		StreamAddr();
		StreamAddr(const std::string& address);
		StreamAddr(const std::string& address, const int port);
		~StreamAddr();
		StreamAddr& operator =(const std::string& address);
}; //class StreamAddr



}; //ns Net
}; //ns R

extern "C" {
#endif /* __cplusplus */


#if __cplusplus
}; //extern "C"
#endif /* __cplusplus */

#endif /* Rasheeq_Net_H_ */
