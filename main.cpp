/**
 * main.cpp
 *
 *  Created on: Aug 4, 2025
 *      Author: swalton
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <alloca.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <sys/wait.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <iostream>
#include <vector>

#include "String.hpp"
using namespace Tools;

extern void unit_tests(void);

enum VerboseLevel {
	eNoVerbosity, eMinimalVerbosity, eMaximalVerbosity
};

/**********************************************************************************************
 * class SocketStreamChannel
 * 		This class encapsulates the internet channel, supplying the I/O for the user.
 */
class SocketStreamChannel {
	int sd;

	struct PingPacket {
		uint16_t packet_num;
		size_t   length;
		uint16_t checksum;
		char     buffer[];
	};

	public:
		SocketStreamChannel(int sd): sd(sd) {}
		virtual ~SocketStreamChannel(void) {
			shutdown(sd, SHUT_WR);
			close(sd);
		}

	public:
		String Receive(int flags = 0) {
			char buffer[20'000];
			memset(buffer, 0, sizeof(buffer));
			recv(sd, buffer, sizeof(buffer), flags);
			return buffer;
		}
		void Send(const String& message, int flags = 0) {
			send(sd, message.GetText(), message.GetLength(), flags);
		}

	public: //--- Tools
		uint16_t Checksum(void *data, int len) {
			uint16_t *buf = (uint16_t*)data, result;
			uint32_t sum = 0;
			for ( sum = 0; len > 1; len -= 2 ) {
				sum += *buf++;
			}
			if ( len == 1 ) {
				sum += *(uint8_t *)buf;
			}
			sum = (sum >> 16) + (sum & 0xFFFF);
			sum += (sum >> 16);
			result = -sum;
			return result;
		}

	public:
		void PingPong(struct sockaddr *addr, socklen_t addrlen) {
			int sd = socket(AF_INET, SOCK_DGRAM, 0);
			char msg[100];
			char peer_addr[100];
			socklen_t peer_addr_len;
			do {
				sendto(sd, "hi", 2, 0, addr, addrlen);
				recvfrom(sd, msg, sizeof(msg), 0, (struct sockaddr*)(&peer_addr), &peer_addr_len);
			} while (true);
		}
		void PongPing(struct sockaddr *addr, socklen_t addrlen) {
			int sd = socket(AF_INET, SOCK_DGRAM, 0);
			char msg[100];
			char peer_addr[100];
			socklen_t peer_addr_len;
			do {
				recvfrom(sd, msg, sizeof(msg), 0, (struct sockaddr*)(&peer_addr), &peer_addr_len);
				sendto(sd, "lo", 2, 0, addr, addrlen);
			} while (true);
		}
};

/**********************************************************************************************
 * struct AddressInfo
 * 		This "public" class abstracts the data from getaddrinfo() -- the preferred method
 * 		for hostname resolution
 */
struct AddressInfo {
	int flags;
	int family;
	int socktype;
	int protocol;
	socklen_t address_len;
	union {
		struct sockaddr* addr;
		struct sockaddr_in* addr_in4;
		struct sockaddr_in6* addr_in6;
	};
	String canonname;
	String address;

	AddressInfo(addrinfo *info, const String& hostname)
			: flags(info->ai_flags), family(info->ai_family), socktype(info->ai_socktype), protocol(info->ai_protocol),
			  address_len(info->ai_addrlen), canonname(hostname)
	{
		char tmps[100];
		void *addr = info->ai_addr;
		address = inet_ntop(family, addr, tmps, info->ai_addrlen);

		if ( family == AF_INET ) {//} == sizeof(struct sockaddr_in) ) {
			addr_in4 = reinterpret_cast<struct sockaddr_in*>(memcpy(new sockaddr_in(), info->ai_addr, address_len));
			// address = String(inet_ntoa(addr_in4->sin_addr));
			// address = inet_ntop(family, (void*)(&reinterpret_cast<struct sockaddr_in*>(info->ai_addr)->sin_addr), tmps, info->ai_addrlen);

		} else if ( family == AF_INET6 ) {//addrlen == sizeof(struct sockaddr_in6) ) {
			addr_in6 = reinterpret_cast<struct sockaddr_in6*>(memcpy(new sockaddr_in6(), info->ai_addr, address_len));
			// address = inet_ntop(family, (void*)(&reinterpret_cast<struct sockaddr_in6*>(info->ai_addr)->sin6_addr), tmps, info->ai_addrlen);

		} else {
			addr = reinterpret_cast<struct sockaddr*>(memcpy(new sockaddr(), info->ai_addr, address_len));
		}
	}
	virtual ~AddressInfo(void) {
		if ( addr != nullptr ) {
			// delete addr;
			addr = nullptr;
		}
	}
	friend std::ostream& operator<<(std::ostream& stream, const AddressInfo& addrinfo) {
		return (stream 	<< "\tname=" << addrinfo.canonname
						<< " flags=" << addrinfo.flags
						<< " family=" << addrinfo.family
						<< " socktype=" << addrinfo.socktype
						<< " protocol=" << addrinfo.protocol
						<< " ip address=" << addrinfo.address);
	}
};

/**********************************************************************************************
 * Global variables for program state.
 */
VerboseLevel mVerboseLevel = eNoVerbosity;
uint16_t mServerPort = 8080;
String mServent = "80";
int mFamily = AF_UNSPEC;
int mSocketType = SOCK_STREAM;
uint32_t mRuntime = 100; //seconds
uint16_t mInterval = 0; // milliseconds
std::vector<String> mHostnames;
std::vector<String> mReportLog;

/**********************************************************************************************
 * String GenerateHttpReport(std::vector<String>& report_log)
 *
 * Purpose:
 * 		This method generates the server-ping log page.
 */
String GenerateHttpReport(std::vector<String>& report_log) {
	FILE *pp = popen("date", "r");
	char *tmps = reinterpret_cast<char*>(alloca(100));
	fgets(tmps, sizeof(tmps), pp);
	pclose(pp);
	String date_str(tmps);
	String http_reply("HTTP/1.1 200 OK\nDate:");
	http_reply += date_str;
	http_reply += "Server: Lone Wolf\nContent-Length: %-8d\n";
	http_reply += "Connection: keep-alive\nContent-Type: text/html;charset=ISO-8859-1\n";
	http_reply += "\n";
	http_reply += "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n<html><head>\n";
	http_reply += "<meta http-equiv=\"refresh\" content=\"10\">\n";
	http_reply += "<title>ActionTarget</title>\n</head>\n<body>";
	for ( String logline : report_log ) {
		http_reply += logline;
	}
	http_reply += "<hr></body>\n</html>\n\n";
	uint report_size = http_reply.GetLength() + 5;
	tmps = reinterpret_cast<char*>(alloca(report_size));
	snprintf(tmps, report_size, http_reply.GetText(), report_size);
	if ( mVerboseLevel > eMinimalVerbosity ) { std::cerr << "Reply: " << tmps; }
	return tmps;
}

/**********************************************************************************************
 * void *ReportServlet(void *arg)
 *
 * Purpose:
 * 		This is the client server. It's only purpose is to supply the HTML log of pinged servers.
 */
void *ReportServlet(void *arg) {
	SocketStreamChannel *channel = reinterpret_cast<SocketStreamChannel*>(arg);
	String request = channel->Receive();
	if ( mVerboseLevel > eMinimalVerbosity ) { std::cerr << "Request: " << request; }

	channel->Send(GenerateHttpReport(mReportLog));
	sleep(1); // <-- needed to get the data out to the client; else, the stream will be dumped.
	delete channel;
	return arg;
}

/**********************************************************************************************
 * void* ReportDispatchServer(void*)
 *
 * Purpose:
 * 		This is a slapped-together HTTP server. The thread:
 * 			> creates a listener socket with a specified port num
 * 			> the setsockopt ensures that if the program fails, the port is not tied up for upto 5 minutes.
 * 			> when a connection is established, the servlet thread is started.
 */
const char *SocketTypeNames[] = {
	"(null)", "Stream", "Datagram", "Raw", "Sequenced, reliable, connection-based, datagrams",
	"Sequenced, reliable, connection-based, datagrams", "Datagram Congestion Control", "(null)",
	"(null)", "(null)", "Packet"
};

bool run = true;
void* ReportDispatchServer(void*) {
	int server_sd;
	if ( (server_sd = socket(AF_INET, SOCK_STREAM, 0)) > 0 ) {
fprintf(stderr, "!!!%s:%d\n", __FILE__, __LINE__);
		socklen_t value = 1;
		if ( setsockopt(server_sd, SOL_SOCKET, SO_REUSEPORT, &value, sizeof(value)) == 0 ) {   // <-- Ensure that the port is "unlocked."
fprintf(stderr, "!!!%s:%d\n", __FILE__, __LINE__);

			struct sockaddr_in addr = { .sin_family = AF_INET, .sin_port = htons(mServerPort) };
			addr.sin_addr.s_addr = INADDR_ANY;
			if ( bind(server_sd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == 0 ) {   // <-- Assign port
fprintf(stderr, "!!!%s:%d Grabbed port #%d\n", __FILE__, __LINE__, mServerPort);

				if ( listen(server_sd, 5) == 0 ) {
fprintf(stderr, "!!!%s:%d Listener\n", __FILE__, __LINE__);
					socklen_t client_addr_size;

					while (run) {
fprintf(stderr, "!!!%s:%d Accept()... wait for connection.\n", __FILE__, __LINE__);
						int client_sd = accept(server_sd, reinterpret_cast<struct sockaddr*>(&addr), &client_addr_size);

fprintf(stderr, "!!!%s:%d Connected!\n", __FILE__, __LINE__);
						if ( mVerboseLevel > eNoVerbosity ) { std::cerr << "Client connection: " << std::endl; }//FIXME: get reentrant address

						pthread_t tid;
						pthread_create(&tid, nullptr, ReportServlet, new SocketStreamChannel(client_sd));
						pthread_detach(tid);
					}

				} else {
					perror("listen()");
				}

			} else {
				perror("bind()");
			}

		} else {
			perror("setsockopt()");
		}

	} else {
		perror("socket()");
	}
	return nullptr;
}

/**********************************************************************************************
 * void *PingService(void *arg)
 * Purpose:
 * 		The thread
 * 			> accepts connection info of an external server,
 * 			> attempts to connect with it,
 * 			> measures the time it takes in milliseconds,
 * 			> generates a report,
 * 			> then closes the connection.
 *
 * @param void *arg - generic reference to AddressInfo.
 * @returns *arg    - which is ignored.
 */
void *PingService(void *arg) {
	AddressInfo host = *reinterpret_cast<AddressInfo*>(arg);

	if ( mVerboseLevel > eMinimalVerbosity ) {
		std::cerr << "Open socket (family=" << host.family << " socket-type=" << host.socktype << ")" << std::endl;
	}

	int sd = socket(host.family, host.socktype, 0);
	if ( sd > 0 ) {
		if ( mVerboseLevel > eNoVerbosity ) {
			std::cerr << host;
		}

	//--- Attempt connection
		int retries = 1000;
		while ( retries-- > 0  &&  connect(sd, host.addr, host.address_len) != 0  &&  errno == EAGAIN ) {
			usleep(100);
		}

	//--- Report string
		std::cerr << "\t" << host.canonname << (retries <= 0? " failure!": " success!") << "(" << (1000 - retries) * 100 << "ms)" << std::endl;
		String log;
		log += "\t";
		log += host.canonname;
		log += (retries <= 0? " failure!": " success!");
		log += "(";
		log += String((1000 - retries) * 1000);
		log += "ms)\n";
		mReportLog.push_back(log);

	//--- Shutdown
		std::cerr.flush();
		shutdown(sd, SHUT_WR);
		close(sd);

	} else {
		perror("socket()");
	}
	return arg;
}

/**********************************************************************************************
 * void ProcessCommandLineArgs(char **args)
 *
 * Purpose:
 * 		This method processes the arguments passed in from the command line.
 *
 * @param char **args - the command line arguments.
 */
void ProcessCommandLineArgs(char **args) {

	while ( *++args != nullptr ) {
		String string(*args);
		if ( string.StartsWith("--port=") ) {
			mServent = String(*args, strlen("--port="));

		} else if ( string == "--IPv4" ) {
			mFamily = AF_INET;

		} else if ( string == "--IPv6" ) {
			mFamily = AF_INET6;

		} else if ( string == "--ICMP" ) {
			std::cerr << "ICMP is not yet implemented.";
			mFamily = AF_PACKET;
			mSocketType = SOCK_RAW;

		} else if ( string == "--UDP" ) {
			mSocketType = SOCK_DGRAM;

		} else if ( string.StartsWith("--verbose=min") ) {
			mVerboseLevel = eMinimalVerbosity;

		} else if ( string.StartsWith("--verbose=max") ) {
			mVerboseLevel = eMaximalVerbosity;

		} else if ( string.StartsWith("--http-port=") ) {
			mServerPort = atoi(String(*args, strlen("--http-port=")).GetText());

		} else if ( string.StartsWith("--runtime=") ) {
			mRuntime = atoi(String(*args, strlen("--runtime=")).GetText());

		} else if ( string.StartsWith("--interval=") ) {
			mInterval = atoi(*args + strlen("--interval="));

		} else if ( string == "--help" ) {
			std::cerr << "[--verbose=[minimal|maximal]] [--runtime=[0-9]+] --port=[[-a-z0-9._/]+|[0-9]+] [--interval=[0-9]+] [--IPv4|--IPv6|--ICMP] [<hostname>|<hostip] [--run-tests]\n";
			exit(0);

		} else if ( string == "--run-tests") {
			unit_tests();
			exit(0);

		} else {
			if ( string.StartsWith("--") ) {
				std::cerr << "Don't know what to do with \"" << string << "\"" << std::endl;
				abort();
			}
			mHostnames.push_back(string);
		}
	}
}

/**********************************************************************************************
 * std::vector<AddressInfo> CollectHosts(std::vector<String> hostnames)
 *
 * Purpose:
 * 		Using getaddrinfo(), map each hostname (or ip address) to an IP. Note: The
 * 		preferred IP-to-text translator is inet_ntop(). I've been encountering
 * 		problems with it in this program; so, I switched back to inet_ntoa() which does
 * 		not support IPv6 addressing.
 *
 * @param std::vector<String> hostnames - a list of names collected from the command line.
 * @return std::vector<AddressInfo> - a list of modified struct addrinfo.
 */
std::vector<AddressInfo> CollectHosts(std::vector<String> hostnames) {
	std::vector<AddressInfo> host_addrs;
	struct addrinfo addrinfo_hint = {
		.ai_flags = (AI_V4MAPPED | AI_ADDRCONFIG | AI_CANONIDN),
		.ai_family = mFamily,
		.ai_socktype = mSocketType
	};
	struct addrinfo *addr_info;

	for ( String hostname : hostnames ) {

		int getaddrinfo_err = getaddrinfo(hostname.GetText(), mServent.GetText(), &addrinfo_hint, &addr_info);
		if ( getaddrinfo_err == 0 ) {

			if ( mVerboseLevel > eNoVerbosity ) {

				for ( struct addrinfo *p = addr_info; p != nullptr; p = p->ai_next ) {
					sockaddr_in *addr = (sockaddr_in*)(p->ai_addr);
					char addr_txt[100];
					inet_ntop(p->ai_family, &addr, addr_txt, p->ai_addrlen);
					// std::cerr << "Family: " << p->ai_family << " name: \"" << hostname << "\" address: " << addr_txt << std::endl;
					std::cerr << "Family: " << p->ai_family << " name: \"" << hostname << std::endl;
				}
			}

			host_addrs.push_back(AddressInfo(addr_info, hostname));
			freeaddrinfo(addr_info);

		} else {
			std::cerr << "Failure: getaddrinfo(" << hostnames[0] << ", " << mServent << "): " << gai_strerror(getaddrinfo_err);
		}
	}
	return host_addrs;
}

/**********************************************************************************************
 * int main(int cnt, char *args[])
 *
 * Purpose:
 * 		Kick off.
 *
 * @param int cnt - number of params + 1
 * @param char *args[] - null-terminated param list.
 */
int main(int cnt, char *args[]) {
//--- interpret command line args
	ProcessCommandLineArgs(args);

//--- Start HTTP server
	pthread_t tid;
	pthread_create(&tid, nullptr, ReportDispatchServer, nullptr);
	pthread_detach(tid);

//--- Collect hosts' addresses
	std::vector<AddressInfo> host_addrs = CollectHosts(mHostnames);

//--- Run ping tests
	if ( host_addrs.size() > 0 ) {

		do {
			std::cerr << "Connecting to... " << std::endl;
			for ( AddressInfo host : host_addrs ) {
				PingService(&host);
			}
			usleep(mInterval * 1000);
		} while ( mInterval > 0 );

		std::cerr.flush();
	}

	sleep(mRuntime);
	run = false;
}

/* Graveyard

unsigned short checksum(void *data, int len) {
	unsigned short *buf = (unsigned short*)data, result;
	unsigned int sum = 0;
	for ( sum = 0; len > 1; len -= 2 ) {
		sum += *buf++;
	}
	if ( len == 1 ) {
		sum += *(unsigned char *)buf;
	}
	sum = (sum >> 16) + (sum & 0xFFFF);
	sum += (sum >> 16);
	result = -sum;
	return result;
}

#define PAYLOAD_SIZE (64 - sizeof(struct icmphdr))
struct RawPacket {
	struct icmphdr header;
	char message[PAYLOAD_SIZE];
};

void RawSocket(struct addrinfo *host) {
	uint cntr = 0;
	RawPacket packet;
	struct protoent *proto = getprotobyname("ICMP");
	int sd = socket(AF_PACKET, SOCK_RAW, proto->p_proto);

	struct sockaddr s_addr;
	socklen_t len = sizeof(s_addr);

	if ( recvfrom(sd, &packet, sizeof(packet), 0, &s_addr, &len) > 0 ) {
		printf("!!!Got message from ");

	} else {
		perror("recvfrom()");
	}

	memset(&packet, 0, sizeof(packet));
	packet.header.type = ICMP_ECHO;
	packet.header.un.echo.id = getpid();
	for ( uint i = 0; i < sizeof(packet.message) - 1; i++ ) {
		packet.message[i] = i + '0';
	}

	packet.message[sizeof(packet.message) - 1] = 0;
	packet.header.un.echo.sequence = cntr++;
	packet.header.checksum = checksum(&packet, sizeof(packet));
	if ( sendto(sd, &packet, sizeof(packet), 0, host->ai_addr, sizeof(host->ai_addrlen)) <= 0 ) {
		perror("sendto");
	}
}
*/