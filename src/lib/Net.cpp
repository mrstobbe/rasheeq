#include "Rasheeq.h"

#warning R::Net is entirely stubbed

R::Net::DatagramAddr::DatagramAddr():
	family_(afIP4),
	protocol_(dgpUDP)
{
};

R::Net::DatagramAddr::DatagramAddr(const std::string& address):
	family_(afIP4),
	protocol_(dgpUDP)
{
	//
};

R::Net::DatagramAddr::DatagramAddr(const std::string& address, const int port):
	family_(afIP4),
	protocol_(dgpUDP)
{
};

R::Net::DatagramAddr::~DatagramAddr() {
};

R::Net::DatagramAddr& R::Net::DatagramAddr::operator =(const std::string& address) {
	//
	return *this;
};



R::Net::StreamAddr::StreamAddr():
	family_(afIP4),
	protocol_(spTCP)
{
};

R::Net::StreamAddr::StreamAddr(const std::string& address):
	family_(afIP4),
	protocol_(spTCP)
{
};

R::Net::StreamAddr::StreamAddr(const std::string& address, const int port):
	family_(afIP4),
	protocol_(spTCP)
{
};

R::Net::StreamAddr::~StreamAddr() {
};

R::Net::StreamAddr& R::Net::StreamAddr::operator =(const std::string& address) {
	//
	return *this;
};

