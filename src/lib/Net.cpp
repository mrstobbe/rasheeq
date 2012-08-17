#include "Rasheeq.h"

#include <sstream>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/un.h>
#if RASHEEQ_HAVE_SCTP
	#include <netinet/sctp.h>
#endif
#include <fcntl.h>
#include <unistd.h>

#warning R::Net is heavily stubbed

R::Net::DatagramAddr::DatagramAddr():
	addr_(NULL),
	addrSize_(0),
	family_(afIP4),
	protocol_(dgpUDP)
{
};

R::Net::DatagramAddr::DatagramAddr(const std::string& address):
	addr_(NULL),
	addrSize_(0),
	family_(afIP4),
	protocol_(dgpUDP)
{
	this->parse(address);
};

R::Net::DatagramAddr::DatagramAddr(const std::string& address, const int port):
	addr_(NULL),
	addrSize_(0),
	family_(afIP4),
	protocol_(dgpUDP)
{
	this->parse(address, port);
};

R::Net::DatagramAddr::DatagramAddr(const int port):
	addr_(NULL),
	addrSize_(0),
	family_(afIP4),
	protocol_(dgpUDP)
{
	this->parse(port);
};

R::Net::DatagramAddr::DatagramAddr(const DatagramAddr& copy):
	addr_(NULL),
	addrSize_(copy.addrSize_),
	family_(copy.family_),
	protocol_(copy.protocol_)
{
	switch (this->family_) {
		case afLocal:
			this->addr_ = new ::sockaddr_un(*reinterpret_cast< ::sockaddr_un* >(this->addr_));
			break;
		case afIP6:
			this->addr_ = new ::sockaddr_in6(*reinterpret_cast< ::sockaddr_in6* >(this->addr_));
			break;
		case afIP4:
			this->addr_ = new ::sockaddr_in(*reinterpret_cast< ::sockaddr_in* >(this->addr_));
			break;
	}
};

R::Net::DatagramAddr::DatagramAddr(DatagramAddr&& move):
	addr_(move.addr_),
	addrSize_(move.addrSize_),
	family_(move.family_),
	protocol_(move.protocol_)
{
	move.reset();
};

R::Net::DatagramAddr::~DatagramAddr() {
	this->reset();
};

R::Net::DatagramAddr& R::Net::DatagramAddr::operator =(const DatagramAddr& copy) {
	if (this == &copy) {
		this->reset();
		this->addr_ = copy.addr_;
		this->addrSize_ = copy.addrSize_;
		this->family_ = copy.family_;
		this->protocol_ = copy.protocol_;
	}
	return *this;
};

R::Net::DatagramAddr& R::Net::DatagramAddr::operator =(DatagramAddr&& move) {
	if (this == &move) {
		this->reset();
		this->addr_ = std::move(move.addr_);
		this->addrSize_ = std::move(move.addrSize_);
		this->family_ = std::move(move.family_);
		this->protocol_ = std::move(move.protocol_);
		move.reset();
	}
	return *this;
};

R::Net::DatagramAddr::operator size_t() const {
	return this->addrSize_;
};

R::Net::DatagramAddr::operator void*() const {
	return this->addr_;
};

R::Net::DatagramAddr& R::Net::DatagramAddr::operator =(const std::string& address) {
	this->parse(address);
	return *this;
};

int R::Net::DatagramAddr::nativeAddrFamily() const {
	switch (this->family_) {
		case afIP6:
			return AF_INET6;
		case afLocal:
			return AF_LOCAL;
		case afIP4:
		default:
			return AF_INET;
	}
};

int R::Net::DatagramAddr::nativeDomain() const {
	#if RASHEEQ_HAVE_SCTP
		if (this->protocol_ == dgpDGSCTP)
			return SOCK_SEQPACKET;
	#endif
	return SOCK_DGRAM;
};

int R::Net::DatagramAddr::nativeProtocol() const {
	#if RASHEEQ_HAVE_SCTP
		if (this->protocol_ == spSCTP)
			return IPPROTO_SCTP;
	#endif
	return 0;
};

void R::Net::DatagramAddr::parse(const std::string& address) {
	AddressFamily af = afIP4;
	DatagramProtocol protocol = dgpUDP;
	int port = 0;

	if (address.size() == 0) {
		//#TODO: Throw error
		return;
	}
	size_t cidx = address.find("://");
	size_t offs = 0;
	if (cidx != std::string::npos) {
		std::string proto = address.substr(0, cidx);
		offs = cidx + 3;
		if (proto == "udp") {
			protocol = dgpUDP;
		//#TODO: Support emulated UDG-like functionality over Windows named pipes
		/*
		} else if ((proto == "local") || (proto == "udg")) {
			protocol = dgpLocal;
		*/
		#if RASHEEQ_HAVE_SCTP
			} else if (proto == "dgsctp") {
				protocol = dgpDGSCTP;
		#endif /* RASHEEQ_HAVE_SCTP */
		} else {
			//#TODO: Throw error
			return;
		}
	}
	if (offs == address.size()) {
		//#TODO: Throw error
		return;
	}

	//#TODO: Support emulated UDG-like functionality over Windows named pipes
	/*
	if (protocol == dgpLocal) {
		af = afLocal;
		#warning local: address parsing is stubbed
	} else */ if (address[offs] == '[') {
		af = afIP6;
		size_t lidx = address.rfind(']');
		if ((lidx == std::string::npos) || (lidx <= offs + 1)) {
			//#TODO: Throw error
			return;
		}
		if (lidx != address.size() - 1) {
			if ((address[lidx + 1] != ':') || (address.size() == lidx + 2)) {
				//#TODO: Throw error
				return;
			}
			if (!(std::stringstream(address.substr(lidx + 2)) >> port)) {
				//#TODO: Throw error
				return;
			}
		}
		std::string ip = address.substr(offs + 1, lidx - (offs + 1));
		::in6_addr ina;
		if (ip == "*") {
			ina = IN6ADDR_ANY_INIT;
		} else if (::inet_pton(AF_INET6, ip.c_str(), &ina) == -1) {
			//#TODO: Throw error
			return;
		}
		::sockaddr_in6* addr = new ::sockaddr_in6;
		addr->sin6_family = AF_INET6;
		addr->sin6_port = htons(port);
		addr->sin6_flowinfo = 0;
		addr->sin6_addr = ina;
		addr->sin6_scope_id = 0;
		this->reset();
		this->addr_ = addr;
		this->addrSize_ = sizeof(::sockaddr_in6);
	} else {
		af = afIP4;
		size_t poidx = address.rfind(':');
		size_t eidx = address.size();
		if ((poidx != std::string::npos) && (poidx > offs)) {
			eidx = poidx;
			if (poidx == address.size() - 1) {
				//#TODO: Throw error
				return;
			}
			if (!(std::stringstream(address.substr(poidx + 1)) >> port)) {
				//#TODO: Throw error
				return;
			}
		}
		std::string ip = address.substr(offs, eidx - offs);
		::in_addr ina;
		if (ip == "*") {
			ina = ::in_addr({ INADDR_ANY });
		} else if (::inet_pton(AF_INET, ip.c_str(), &ina) == -1) {
			//#TODO: Throw error
			return;
		}
		::sockaddr_in* addr = new ::sockaddr_in;
		addr->sin_family = AF_INET;
		addr->sin_port = htons(port);
		addr->sin_addr = ina;
		this->reset();
		this->addr_ = addr;
		this->addrSize_ = sizeof(::sockaddr_in);
	}
	this->family_ = af;
	this->protocol_ = protocol;
};

void R::Net::DatagramAddr::parse(const std::string& address, const int port) {
};

void R::Net::DatagramAddr::parse(const int port) {
	::sockaddr_in* addr = new ::sockaddr_in;
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
	addr->sin_addr = ::in_addr({ INADDR_ANY });
	this->reset();
	this->addr_ = addr;
	this->addrSize_ = sizeof(::sockaddr_in);
	this->family_ = afIP4;
	this->protocol_ = dgpUDP;
};

void R::Net::DatagramAddr::reset() {
	if (this->addr_ != NULL) {
		switch (this->family_) {
			case afLocal:
				delete reinterpret_cast< ::sockaddr_un* >(this->addr_);
				break;
			case afIP6:
				delete reinterpret_cast< ::sockaddr_in6* >(this->addr_);
				break;
			case afIP4:
				delete reinterpret_cast< ::sockaddr_in* >(this->addr_);
				break;
		}
		this->addr_ = NULL;
		this->addrSize_ = 0;
	}
	this->family_ = afIP4;
	this->protocol_ = dgpUDP;
};



R::Net::StreamAddr::StreamAddr():
	addr_(NULL),
	addrSize_(0),
	family_(afIP4),
	protocol_(spTCP)
{
};

R::Net::StreamAddr::StreamAddr(const std::string& address):
	addr_(NULL),
	addrSize_(0),
	family_(afIP4),
	protocol_(spTCP)
{
	this->parse(address);
};

R::Net::StreamAddr::StreamAddr(const std::string& address, const int port):
	addr_(NULL),
	addrSize_(0),
	family_(afIP4),
	protocol_(spTCP)
{
	this->parse(address);
};

R::Net::StreamAddr::StreamAddr(const int port):
	addr_(NULL),
	addrSize_(0),
	family_(afIP4),
	protocol_(spTCP)
{
	this->parse(port);
};

R::Net::StreamAddr::StreamAddr(const StreamAddr& copy):
	addr_(NULL),
	addrSize_(copy.addrSize_),
	family_(copy.family_),
	protocol_(copy.protocol_)
{
	switch (this->family_) {
		case afLocal:
			this->addr_ = new ::sockaddr_un(*reinterpret_cast< ::sockaddr_un* >(this->addr_));
			break;
		case afIP6:
			this->addr_ = new ::sockaddr_in6(*reinterpret_cast< ::sockaddr_in6* >(this->addr_));
			break;
		case afIP4:
			this->addr_ = new ::sockaddr_in(*reinterpret_cast< ::sockaddr_in* >(this->addr_));
			break;
	}
};

R::Net::StreamAddr::StreamAddr(StreamAddr&& move):
	addr_(move.addr_),
	addrSize_(move.addrSize_),
	family_(move.family_),
	protocol_(move.protocol_)
{
	move.reset();
};

R::Net::StreamAddr::~StreamAddr() {
	this->reset();
};

R::Net::StreamAddr& R::Net::StreamAddr::operator =(const StreamAddr& copy) {
	if (this == &copy) {
		this->reset();
		this->addr_ = copy.addr_;
		this->addrSize_ = copy.addrSize_;
		this->family_ = copy.family_;
		this->protocol_ = copy.protocol_;
	}
	return *this;
};

R::Net::StreamAddr& R::Net::StreamAddr::operator =(StreamAddr&& move) {
	if (this == &move) {
		this->reset();
		this->addr_ = std::move(move.addr_);
		this->addrSize_ = std::move(move.addrSize_);
		this->family_ = std::move(move.family_);
		this->protocol_ = std::move(move.protocol_);
		move.reset();
	}
	return *this;
};

R::Net::StreamAddr& R::Net::StreamAddr::operator =(const std::string& address) {
	this->parse(address);
	return *this;
};

R::Net::StreamAddr::operator size_t() const {
	return this->addrSize_;
};

R::Net::StreamAddr::operator void*() const {
	return this->addr_;
};

int R::Net::StreamAddr::nativeAddrFamily() const {
	switch (this->family_) {
		case afIP6:
			return AF_INET6;
		case afLocal:
			return AF_LOCAL;
		case afIP4:
		default:
			return AF_INET;
	}
};

int R::Net::StreamAddr::nativeDomain() const {
	return SOCK_STREAM;
};

int R::Net::StreamAddr::nativeProtocol() const {
	#if RASHEEQ_HAVE_SCTP
		if (this->protocol_ == spSCTP)
			return IPPROTO_SCTP;
	#endif
	return 0;
};

/*
if protocol:// is omitted, tcp:// is assumed
tcp:// *
tcp:// *:1234
tcp://127.0.0.1
tcp://127.0.0.1:1234
tcp://[*]:1234
tcp://[::1]:1234
local:///var/lib/some.sock
local:///var/lib/some.sock:1234
sctp://127.0.0.1:1234
*/

void R::Net::StreamAddr::parse(const std::string& address) {
	AddressFamily af = afIP4;
	StreamProtocol protocol = spTCP;
	int port = 0;

	if (address.size() == 0) {
		//#TODO: Throw error
		return;
	}
	size_t cidx = address.find("://");
	size_t offs = 0;
	if (cidx != std::string::npos) {
		std::string proto = address.substr(0, cidx);
		offs = cidx + 3;
		if (proto == "tcp") {
			protocol = spTCP;
		} else if (proto == "local") {
			protocol = spLocal;
		#if RASHEEQ_HAVE_SCTP
			} else if (proto == "sctp") {
				protocol = spSCTP;
		#endif /* RASHEEQ_HAVE_SCTP */
		} else {
			//#TODO: Throw error
			return;
		}
	}
	if (offs == address.size()) {
		//#TODO: Throw error
		return;
	}
	if (protocol == spLocal) {
		af = afLocal;
		#warning local: address parsing is stubbed
	} else if (address[offs] == '[') {
		af = afIP6;
		size_t lidx = address.rfind(']');
		if ((lidx == std::string::npos) || (lidx <= offs + 1)) {
			//#TODO: Throw error
			return;
		}
		if (lidx != address.size() - 1) {
			if ((address[lidx + 1] != ':') || (address.size() == lidx + 2)) {
				//#TODO: Throw error
				return;
			}
			if (!(std::stringstream(address.substr(lidx + 2)) >> port)) {
				//#TODO: Throw error
				return;
			}
		}
		std::string ip = address.substr(offs + 1, lidx - (offs + 1));
		::in6_addr ina;
		if (ip == "*") {
			ina = IN6ADDR_ANY_INIT;
		} else if (::inet_pton(AF_INET6, ip.c_str(), &ina) == -1) {
			//#TODO: Throw error
			return;
		}
		::sockaddr_in6* addr = new ::sockaddr_in6;
		addr->sin6_family = AF_INET6;
		addr->sin6_port = htons(port);
		addr->sin6_flowinfo = 0;
		addr->sin6_addr = ina;
		addr->sin6_scope_id = 0;
		this->reset();
		this->addr_ = addr;
		this->addrSize_ = sizeof(::sockaddr_in6);
	} else {
		af = afIP4;
		size_t poidx = address.rfind(':');
		size_t eidx = address.size();
		if ((poidx != std::string::npos) && (poidx > offs)) {
			eidx = poidx;
			if (poidx == address.size() - 1) {
				//#TODO: Throw error
				return;
			}
			if (!(std::stringstream(address.substr(poidx + 1)) >> port)) {
				//#TODO: Throw error
				return;
			}
		}
		std::string ip = address.substr(offs, eidx - offs);
		::in_addr ina;
		if (ip == "*") {
			ina = ::in_addr({ INADDR_ANY });
		} else if (::inet_pton(AF_INET, ip.c_str(), &ina) == -1) {
			//#TODO: Throw error
			return;
		}
		::sockaddr_in* addr = new ::sockaddr_in;
		addr->sin_family = AF_INET;
		addr->sin_port = htons(port);
		addr->sin_addr = ina;
		this->reset();
		this->addr_ = addr;
		this->addrSize_ = sizeof(::sockaddr_in);
	}
	this->family_ = af;
	this->protocol_ = protocol;
};

void R::Net::StreamAddr::parse(const std::string& address, const int port) {
};

void R::Net::StreamAddr::parse(const int port) {
	::sockaddr_in* addr = new ::sockaddr_in;
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
	addr->sin_addr = ::in_addr({ INADDR_ANY });
	this->reset();
	this->addr_ = addr;
	this->addrSize_ = sizeof(::sockaddr_in);
	this->family_ = afIP4;
	this->protocol_ = spTCP;
};

void R::Net::StreamAddr::reset() {
	if (this->addr_ != NULL) {
		switch (this->family_) {
			case afLocal:
				delete reinterpret_cast< ::sockaddr_un* >(this->addr_);
				break;
			case afIP6:
				delete reinterpret_cast< ::sockaddr_in6* >(this->addr_);
				break;
			case afIP4:
				delete reinterpret_cast< ::sockaddr_in* >(this->addr_);
				break;
		}
		this->addr_ = NULL;
		this->addrSize_ = 0;
	}
	this->family_ = afIP4;
	this->protocol_ = spTCP;
};
