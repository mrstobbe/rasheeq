#include <iostream>

#include <Rasheeq/Poller.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

using namespace std;


int main(int argc, char** argv) {
	int sock = ::socket(AF_INET, SOCK_STREAM, 0);
	
};
