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
	address_("*"),
	family_(afIP4),
	nativeAddr_(NULL),
	nativeAddrSize_(0),
	port_(0),
	protocol_(dgpUDP)
{
};

R::Net::DatagramAddr::DatagramAddr(const DatagramProtocol protocol, void* nativeAddress):
	address_("*"),
	family_(afIP4),
	nativeAddr_(NULL),
	nativeAddrSize_(0),
	port_(0),
	protocol_(protocol)
{
	int af = *reinterpret_cast<short*>(nativeAddress);
	switch (af) {
		case AF_INET: {
				::sockaddr_in* addr = reinterpret_cast< ::sockaddr_in* >(nativeAddress);
				std::string ip = "*";
				if (addr->sin_addr.s_addr != INADDR_ANY) {
					char buf[INET_ADDRSTRLEN];
					if (::inet_ntop(AF_INET, &addr->sin_addr, buf, INET_ADDRSTRLEN) == NULL) {
						//#TODO: Throw exception
					}
					ip = buf;
				}
				this->address_ = ip;
				this->family_ = afIP4;
				this->nativeAddr_ = new ::sockaddr_in(*addr);
				this->nativeAddrSize_ = sizeof(::sockaddr_in);
				this->port_ = ntohs(addr->sin_port);
			} break;
		case AF_INET6: {
				char buf[INET6_ADDRSTRLEN];
				::sockaddr_in6* addr = reinterpret_cast< ::sockaddr_in6* >(nativeAddress);
				if (::inet_ntop(AF_INET6, &addr->sin6_addr, buf, INET6_ADDRSTRLEN) == NULL) {
					//#TODO: Throw exception
				}
				std::string ip = buf;
				if (ip == "::")
					ip = "*";
				this->address_ = ip;
				this->family_ = afIP6;
				this->nativeAddr_ = new ::sockaddr_in6(*addr);
				this->nativeAddrSize_ = sizeof(::sockaddr_in6);
				this->port_ = ntohs(addr->sin6_port);
			} break;
		case AF_LOCAL: {
				::sockaddr_un* addr = reinterpret_cast< ::sockaddr_un* >(nativeAddress);
				std::string path = addr->sun_path;
				size_t pi = path.rfind('/');
				size_t oi = path.rfind(':');
				if ((pi != std::string::npos) && (oi != std::string::npos) && (oi < pi))
					oi = std::string::npos;
				int port = 0;
				if ((oi != std::string::npos) && (oi != path.size() - 1) && (!!(std::stringstream(path.substr(oi + 1)) >> port)))
					path.erase(oi);
				this->address_ = path;
				this->family_ = afLocal;
				this->nativeAddr_ = new ::sockaddr_un(*addr);
				this->nativeAddrSize_ = sizeof(::sockaddr_un);
				this->port_ = port;
			} break;
		default:
			//#TODO: Throw exception
			break;
	}
};

R::Net::DatagramAddr::DatagramAddr(const std::string& address):
	address_("*"),
	family_(afIP4),
	nativeAddr_(NULL),
	nativeAddrSize_(0),
	port_(0),
	protocol_(dgpUDP)
{
	this->parse(address);
};

R::Net::DatagramAddr::DatagramAddr(const std::string& address, const int port):
	address_("*"),
	family_(afIP4),
	nativeAddr_(NULL),
	nativeAddrSize_(0),
	port_(0),
	protocol_(dgpUDP)
{
	this->parse(address, port);
};

R::Net::DatagramAddr::DatagramAddr(const int port):
	address_("*"),
	family_(afIP4),
	nativeAddr_(NULL),
	nativeAddrSize_(0),
	port_(0),
	protocol_(dgpUDP)
{
	this->parse(port);
};

R::Net::DatagramAddr::DatagramAddr(const DatagramAddr& copy):
	address_(copy.address_),
	family_(copy.family_),
	nativeAddr_(NULL),
	nativeAddrSize_(copy.nativeAddrSize_),
	port_(copy.port_),
	protocol_(copy.protocol_)
{
	switch (this->family_) {
		case afLocal:
			this->nativeAddr_ = new ::sockaddr_un(*reinterpret_cast< ::sockaddr_un* >(copy.nativeAddr_));
			break;
		case afIP6:
			this->nativeAddr_ = new ::sockaddr_in6(*reinterpret_cast< ::sockaddr_in6* >(copy.nativeAddr_));
			break;
		case afIP4:
			this->nativeAddr_ = new ::sockaddr_in(*reinterpret_cast< ::sockaddr_in* >(copy.nativeAddr_));
			break;
	}
};

R::Net::DatagramAddr::DatagramAddr(DatagramAddr&& move):
	address_(std::move(move.address_)),
	family_(std::move(move.family_)),
	nativeAddr_(std::move(move.nativeAddr_)),
	nativeAddrSize_(std::move(move.nativeAddrSize_)),
	port_(std::move(move.port_)),
	protocol_(std::move(move.protocol_))
{
	move.nativeAddr_ = NULL;
};

R::Net::DatagramAddr::~DatagramAddr() {
	this->reset();
};

R::Net::DatagramAddr& R::Net::DatagramAddr::operator =(const DatagramAddr& copy) {
	if (this != &copy) {
		this->reset();
		this->address_ = copy.address_;
		this->family_ = copy.family_;
		if (copy.nativeAddr_ != NULL) {
			switch (this->family_) {
				case afLocal:
					this->nativeAddr_ = new ::sockaddr_un(*reinterpret_cast< ::sockaddr_un* >(copy.nativeAddr_));
					break;
				case afIP6:
					this->nativeAddr_ = new ::sockaddr_in6(*reinterpret_cast< ::sockaddr_in6* >(copy.nativeAddr_));
					break;
				case afIP4:
					this->nativeAddr_ = new ::sockaddr_in(*reinterpret_cast< ::sockaddr_in* >(copy.nativeAddr_));
					break;
			}
		}
		this->nativeAddr_ = copy.nativeAddr_;
		this->port_ = copy.port_;
		this->protocol_ = copy.protocol_;
	}
	return *this;
};

R::Net::DatagramAddr& R::Net::DatagramAddr::operator =(DatagramAddr&& move) {
	if (this != &move) {
		this->reset();
		this->address_ = std::move(move.address_);
		this->family_ = std::move(move.family_);
		this->nativeAddr_ = std::move(move.nativeAddr_);
		this->nativeAddrSize_ = std::move(move.nativeAddrSize_);
		this->port_ = std::move(move.port_);
		this->protocol_ = std::move(move.protocol_);
		move.nativeAddr_ = NULL;
	}
	return *this;
};

R::Net::DatagramAddr::operator size_t() const {
	return this->nativeAddrSize_;
};

R::Net::DatagramAddr::operator void*() const {
	return this->nativeAddr_;
};

R::Net::DatagramAddr& R::Net::DatagramAddr::operator =(const std::string& address) {
	this->parse(address);
	return *this;
};

std::string R::Net::DatagramAddr::address() const {
	return this->address_;
};

R::Net::AddressFamily R::Net::DatagramAddr::family() const {
	return this->family_;
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

int R::Net::DatagramAddr::port() const {
	return this->port_;
};

R::Net::DatagramProtocol R::Net::DatagramAddr::protocol() const {
	return this->protocol_;
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
		bool isWild = ((ip == "*") || (ip == "::"));
		::in6_addr ina;
		if (isWild) {
			ina = IN6ADDR_ANY_INIT;
			ip = "*";
		} else if (::inet_pton(AF_INET6, ip.c_str(), &ina) == -1) {
			//#TODO: Throw error
			return;
		}
		if (!isWild) {
			isWild = true;
			for (size_t i = 0; i < 16; ++i) {
				if (ina.s6_addr[i]) {
					isWild = false;
					break;
				}
			}
			if (isWild)
				ip = "*";
		}
		::sockaddr_in6* addr = new ::sockaddr_in6;
		addr->sin6_family = AF_INET6;
		addr->sin6_port = htons(port);
		addr->sin6_flowinfo = 0;
		addr->sin6_addr = ina;
		addr->sin6_scope_id = 0;
		this->reset();
		this->address_ = ip;
		this->nativeAddr_ = addr;
		this->nativeAddrSize_ = sizeof(::sockaddr_in6);
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
		bool isWild = (ip == "*");
		::in_addr ina;
		if (isWild) {
			ina = ::in_addr({ INADDR_ANY });
		} else if (::inet_pton(AF_INET, ip.c_str(), &ina) == -1) {
			//#TODO: Throw error
			return;
		}
		if ((!isWild) && (ina.s_addr == INADDR_ANY))
			ip = "*";
		::sockaddr_in* addr = new ::sockaddr_in;
		addr->sin_family = AF_INET;
		addr->sin_port = htons(port);
		addr->sin_addr = ina;
		this->reset();
		this->address_ = ip;
		this->nativeAddr_ = addr;
		this->nativeAddrSize_ = sizeof(::sockaddr_in);
	}
	this->family_ = af;
	this->port_ = port;
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
	this->address_ = "*";
	this->nativeAddr_ = addr;
	this->nativeAddrSize_ = sizeof(::sockaddr_in);
	this->family_ = afIP4;
	this->port_ = port;
	this->protocol_ = dgpUDP;
};

void R::Net::DatagramAddr::reset() {
	this->address_ = "*";
	if (this->nativeAddr_ != NULL) {
		switch (this->family_) {
			case afLocal:
				delete reinterpret_cast< ::sockaddr_un* >(this->nativeAddr_);
				break;
			case afIP6:
				delete reinterpret_cast< ::sockaddr_in6* >(this->nativeAddr_);
				break;
			case afIP4:
				delete reinterpret_cast< ::sockaddr_in* >(this->nativeAddr_);
				break;
		}
		this->nativeAddr_ = NULL;
		this->nativeAddrSize_ = 0;
	}
	this->family_ = afIP4;
	this->port_ = 0;
	this->protocol_ = dgpUDP;
};



R::Net::StreamAddr::StreamAddr():
	address_("*"),
	family_(afIP4),
	nativeAddr_(NULL),
	nativeAddrSize_(0),
	port_(0),
	protocol_(spTCP)
{
};

R::Net::StreamAddr::StreamAddr(const StreamProtocol protocol, void* nativeAddress):
	address_("*"),
	family_(afIP4),
	nativeAddr_(NULL),
	nativeAddrSize_(0),
	port_(0),
	protocol_(protocol)
{
	int af = *reinterpret_cast<short*>(nativeAddress);
	switch (af) {
		case AF_INET: {
				::sockaddr_in* addr = reinterpret_cast< ::sockaddr_in* >(nativeAddress);
				std::string ip = "*";
				if (addr->sin_addr.s_addr != INADDR_ANY) {
					char buf[INET_ADDRSTRLEN];
					if (::inet_ntop(AF_INET, &addr->sin_addr, buf, INET_ADDRSTRLEN) == NULL) {
						//#TODO: Throw exception
					}
					ip = buf;
				}
				this->address_ = ip;
				this->family_ = afIP4;
				this->nativeAddr_ = new ::sockaddr_in(*addr);
				this->nativeAddrSize_ = sizeof(::sockaddr_in);
				this->port_ = ntohs(addr->sin_port);
			} break;
		case AF_INET6: {
				::sockaddr_in6* addr = reinterpret_cast< ::sockaddr_in6* >(nativeAddress);
				char buf[INET6_ADDRSTRLEN];
				if (::inet_ntop(AF_INET6, &addr->sin6_addr, buf, INET6_ADDRSTRLEN) == NULL) {
					//#TODO: Throw exception
				}
				std::string ip = buf;
				if (ip == "::")
					ip = "*";
				this->address_ = ip;
				this->family_ = afIP6;
				this->nativeAddr_ = new ::sockaddr_in6(*addr);
				this->nativeAddrSize_ = sizeof(::sockaddr_in6);
				this->port_ = ntohs(addr->sin6_port);
			} break;
		case AF_LOCAL: {
				::sockaddr_un* addr = reinterpret_cast< ::sockaddr_un* >(nativeAddress);
				std::string path = addr->sun_path;
				size_t pi = path.rfind('/');
				size_t oi = path.rfind(':');
				if ((pi != std::string::npos) && (oi != std::string::npos) && (oi < pi))
					oi = std::string::npos;
				int port = 0;
				if ((oi != std::string::npos) && (oi != path.size() - 1) && (!!(std::stringstream(path.substr(oi + 1)) >> port)))
					path.erase(oi);
				this->address_ = path;
				this->family_ = afLocal;
				this->nativeAddr_ = new ::sockaddr_un(*addr);
				this->nativeAddrSize_ = sizeof(::sockaddr_un);
				this->port_ = port;
			} break;
		default:
			//#TODO: Throw exception
			break;
	}
};

R::Net::StreamAddr::StreamAddr(const std::string& address):
	address_("*"),
	family_(afIP4),
	nativeAddr_(NULL),
	nativeAddrSize_(0),
	port_(0),
	protocol_(spTCP)
{
	this->parse(address);
};

R::Net::StreamAddr::StreamAddr(const std::string& address, const int port):
	address_("*"),
	family_(afIP4),
	nativeAddr_(NULL),
	nativeAddrSize_(0),
	port_(0),
	protocol_(spTCP)
{
	this->parse(address);
};

R::Net::StreamAddr::StreamAddr(const int port):
	address_("*"),
	family_(afIP4),
	nativeAddr_(NULL),
	nativeAddrSize_(0),
	port_(0),
	protocol_(spTCP)
{
	this->parse(port);
};

R::Net::StreamAddr::StreamAddr(const StreamAddr& copy):
	address_(copy.address_),
	family_(copy.family_),
	nativeAddr_(NULL),
	nativeAddrSize_(copy.nativeAddrSize_),
	port_(copy.port_),
	protocol_(copy.protocol_)
{
	switch (this->family_) {
		case afLocal:
			this->nativeAddr_ = new ::sockaddr_un(*reinterpret_cast< ::sockaddr_un* >(copy.nativeAddr_));
			break;
		case afIP6:
			this->nativeAddr_ = new ::sockaddr_in6(*reinterpret_cast< ::sockaddr_in6* >(copy.nativeAddr_));
			break;
		case afIP4:
			this->nativeAddr_ = new ::sockaddr_in(*reinterpret_cast< ::sockaddr_in* >(copy.nativeAddr_));
			break;
	}
};

R::Net::StreamAddr::StreamAddr(StreamAddr&& move):
	address_(std::move(move.address_)),
	family_(std::move(move.family_)),
	nativeAddr_(std::move(move.nativeAddr_)),
	nativeAddrSize_(std::move(move.nativeAddrSize_)),
	port_(std::move(move.port_)),
	protocol_(std::move(move.protocol_))
{
	move.nativeAddr_ = NULL;
};

R::Net::StreamAddr::~StreamAddr() {
	this->reset();
};

R::Net::StreamAddr& R::Net::StreamAddr::operator =(const StreamAddr& copy) {
	if (this != &copy) {
		this->reset();
		this->address_ = copy.address_;
		this->family_ = copy.family_;
		if (copy.nativeAddr_ != NULL) {
			switch (this->family_) {
				case afLocal:
					this->nativeAddr_ = new ::sockaddr_un(*reinterpret_cast< ::sockaddr_un* >(copy.nativeAddr_));
					break;
				case afIP6:
					this->nativeAddr_ = new ::sockaddr_in6(*reinterpret_cast< ::sockaddr_in6* >(copy.nativeAddr_));
					break;
				case afIP4:
					this->nativeAddr_ = new ::sockaddr_in(*reinterpret_cast< ::sockaddr_in* >(copy.nativeAddr_));
					break;
			}
		}
		this->nativeAddrSize_ = copy.nativeAddrSize_;
		this->port_ = copy.port_;
		this->protocol_ = copy.protocol_;
	}
	return *this;
};

R::Net::StreamAddr& R::Net::StreamAddr::operator =(StreamAddr&& move) {
	if (this != &move) {
		this->reset();
		this->address_ = std::move(move.address_);
		this->family_ = std::move(move.family_);
		this->nativeAddr_ = std::move(move.nativeAddr_);
		this->nativeAddrSize_ = std::move(move.nativeAddrSize_);
		this->port_ = std::move(move.port_);
		this->protocol_ = std::move(move.protocol_);
		move.nativeAddr_ = NULL;
	}
	return *this;
};

R::Net::StreamAddr& R::Net::StreamAddr::operator =(const std::string& address) {
	this->parse(address);
	return *this;
};

R::Net::StreamAddr::operator std::string() const {
	return this->identity();
};

R::Net::StreamAddr::operator size_t() const {
	return this->nativeAddrSize_;
};

R::Net::StreamAddr::operator void*() const {
	return this->nativeAddr_;
};

std::string R::Net::StreamAddr::address() const {
	return this->address_;
};

R::Net::AddressFamily R::Net::StreamAddr::family() const {
	return this->family_;
};

std::string R::Net::StreamAddr::identity() const {
	std::stringstream res;
	switch (this->protocol_) {
		case spLocal:
			res << "local";
			break;
		#if RASHEEQ_HAVE_SCTP
			case spSCTP:
				res << "sctp";
				break;
		#endif /* RASHEEQ_HAVE_SCTP */
		case spTCP:
		default:
			res << "tcp";
			break;
	}
	res << "://";
	if (this->family_ == afIP6) {
		res << "[" << this->address_ << "]";
	} else {
		res << this->address_;
	}
	res << ":" << this->port_;
	return res.str();
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

int R::Net::StreamAddr::port() const {
	return this->port_;
};

R::Net::StreamProtocol R::Net::StreamAddr::protocol() const {
	return this->protocol_;
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
		bool isWild = ((ip == "*") || (ip == "::"));
		::in6_addr ina;
		if (isWild) {
			ina = IN6ADDR_ANY_INIT;
			ip = "*";
		} else if (::inet_pton(AF_INET6, ip.c_str(), &ina) == -1) {
			//#TODO: Throw error
			return;
		}
		if (!isWild) {
			isWild = true;
			for (size_t i = 0; i < 16; ++i) {
				if (ina.s6_addr[i]) {
					isWild = false;
					break;
				}
			}
			if (isWild)
				ip = "*";
		}
		::sockaddr_in6* addr = new ::sockaddr_in6;
		addr->sin6_family = AF_INET6;
		addr->sin6_port = htons(port);
		addr->sin6_flowinfo = 0;
		addr->sin6_addr = ina;
		addr->sin6_scope_id = 0;
		this->reset();
		this->address_ = ip;
		this->nativeAddr_ = addr;
		this->nativeAddrSize_ = sizeof(::sockaddr_in6);
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
		bool isWild = (ip == "*");
		::in_addr ina;
		if (isWild) {
			ina = ::in_addr({ INADDR_ANY });
		} else if (::inet_pton(AF_INET, ip.c_str(), &ina) == -1) {
			//#TODO: Throw error
			return;
		}
		if ((!isWild) && (ina.s_addr == INADDR_ANY))
			ip = "*";
		::sockaddr_in* addr = new ::sockaddr_in;
		addr->sin_family = AF_INET;
		addr->sin_port = htons(port);
		addr->sin_addr = ina;
		this->reset();
		this->address_ = ip;
		this->nativeAddr_ = addr;
		this->nativeAddrSize_ = sizeof(::sockaddr_in);
	}
	this->family_ = af;
	this->port_ = port;
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
	this->address_ = "*";
	this->nativeAddr_ = addr;
	this->nativeAddrSize_ = sizeof(::sockaddr_in);
	this->family_ = afIP4;
	this->port_ = port;
	this->protocol_ = spTCP;
};

void R::Net::StreamAddr::reset() {
	this->address_ = "*";
	if (this->nativeAddr_ != NULL) {
		switch (this->family_) {
			case afLocal:
				delete reinterpret_cast< ::sockaddr_un* >(this->nativeAddr_);
				break;
			case afIP6:
				delete reinterpret_cast< ::sockaddr_in6* >(this->nativeAddr_);
				break;
			case afIP4:
				delete reinterpret_cast< ::sockaddr_in* >(this->nativeAddr_);
				break;
		}
		this->nativeAddr_ = NULL;
		this->nativeAddrSize_ = 0;
	}
	this->family_ = afIP4;
	this->port_ = 0;
	this->protocol_ = spTCP;
};
