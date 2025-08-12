/**
 * main.cpp
 *
 *  Created on: Aug 4, 2025
 *      Author: swalton
 *
 * Copyright (c) 2025, IAS Publishing, LLC
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

#define MAX_PEGBOARD 100
#define VERBOSE(tier, cmd) if (mVerboseLevel > tier) {cmd};

#include "String.hpp"
using namespace Tools;

extern void unit_tests(void);
typedef unsigned char byte;

enum VerboseLevel {
	eNoVerbosity, eMinimalVerbosity, eMaximalVerbosity
};

extern const char *mClientHtmlPage;
extern const char *mSocketTypeNames[];
extern const char *mSocketFamilyNames[];
pthread_t tid;
pthread_attr_t mThreadAttributes;

/**********************************************************************************************
 * Global variables for program state.
 */
VerboseLevel mVerboseLevel = eNoVerbosity;
uint16_t mServerPort = 8080;
String mServent = "80";
int mFamily = AF_UNSPEC;
int mSocketType = SOCK_STREAM;
int mRuntime = 10; //seconds
uint16_t mInterval = 0; // milliseconds
pthread_mutex_t mReportLog_mutex = PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP;
bool mRun = true;
bool mUDP_Client = true;
std::vector<String> mHostnames;
std::vector<String> mReportLog;

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
		void *ai_addr = info->ai_addr;
		address = inet_ntop(family, ai_addr, tmps, info->ai_addrlen);

		if ( family == AF_INET ) {
			addr_in4 = reinterpret_cast<struct sockaddr_in*>(memcpy(new sockaddr_in(), info->ai_addr, address_len));

		} else if ( family == AF_INET6 ) {
			addr_in6 = reinterpret_cast<struct sockaddr_in6*>(memcpy(new sockaddr_in6(), info->ai_addr, address_len));

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
		return (stream 	<< "\tName=" << addrinfo.canonname << " "
						<< "Flags=\"" << addrinfo.flags << "\" "
						<< "Family=\"" << mSocketFamilyNames[addrinfo.family] << "\" "
						<< "Socktype=\"" << mSocketTypeNames[addrinfo.socktype] << "\" "
						<< "Protocol=\"" << getprotobynumber(addrinfo.protocol)->p_name << "\" "
						<< "IP-address=\"" << addrinfo.address) << "\"";
	}
};

/**********************************************************************************************
 * class SocketStreamChannel
 * 		This class encapsulates the internet channel, supplying the I/O for the user.
 */
class SocketStreamChannel {
	int sd;

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
		static uint16_t Checksum(void *data, int len) {
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
		enum PegboardState: byte { eUndefined, eSent, eReceived_okay, eReceived_fail }; // <-- force each element to be byte-size
		struct PingPacket {
			uint16_t 		packet_num;
			size_t   		packet_size;
			time_t	 		timestamp;
			uint16_t 		checksum;
			PegboardState 	state;
			char     		buffer[]; 	// <-- an old-C trick: you can actually leave the last element (which must be an
										//		array) without a size, making it possible to have a dynamically sized struct.
		};

/**********************************************************************************************
 * std::vector<AddressInfo> CollectHosts(std::vector<String> hostnames)
 *
 * Purpose:
 * 		Using getaddrinfo(), map each hostname (or ip address) to an IP. Note: The
 * 		preferred IP-to-text translator is inet_ntop(). I've been encountering
 * 		problems with it in this program; so, I switched back to inet_ntoa() which does
 * 		not support IPv6 addressing.
 *
 * BASIC ALGORITHM
 * 		1) for every hostname
 * 			a) look up name through getaddrinfo()
 * 			b) display if verbose
 * 			c) add to array
 *
 * @param std::vector<String> hostnames - a list of names collected from the command line.
 * @return std::vector<AddressInfo> - a list of modified struct addrinfo.
 */
		static std::vector<AddressInfo> CollectHosts(std::vector<String> hostnames, int inet_family, int socket_type) {
			std::vector<AddressInfo> host_addrs;
			struct addrinfo addrinfo_hint = {
				.ai_flags = (AI_V4MAPPED | AI_ADDRCONFIG | AI_CANONIDN),
				.ai_family = inet_family,
				.ai_socktype = socket_type
			};
			struct addrinfo *addr_info;

			for ( String hostname : hostnames ) {

				int getaddrinfo_err = getaddrinfo(hostname.GetText(), mServent.GetText(), &addrinfo_hint, &addr_info);
				if ( getaddrinfo_err == 0 ) {

					VERBOSE(eNoVerbosity, {
						for ( struct addrinfo *p = addr_info; p != nullptr; p = p->ai_next ) {
							// sockaddr_in *addr = (sockaddr_in*)(p->ai_addr);
							char addr_txt[100];
							inet_ntop(p->ai_family, p->ai_addr, addr_txt, p->ai_addrlen);
							std::cout << "Family: " << p->ai_family << " name: \"" << hostname << "\" address: " << addr_txt << std::endl;
							std::cout << "Family: " << p->ai_family << " name: \"" << hostname << std::endl;
						}
					});

					host_addrs.push_back(AddressInfo(addr_info, hostname));
					freeaddrinfo(addr_info);

				} else {
					std::cout << "Failure: getaddrinfo(" << hostnames[0] << ", " << mServent << "): " << gai_strerror(getaddrinfo_err);
				}
			}
			return host_addrs;
		}

/** void PingPong(struct sockaddr *addr, socklen_t addrlen, uint frequency_us = 100'000)
 *		Send a datagram to a peer, await a reply, check message integrity.
 *
 * __GENERAL_ALGORITHM___
 * 	* Create a thread which will:
 * 		+ Listen for a message
 * 		+ Check peer's report
 * 		+ Verify integrity
 * 		+ Check off pegboard
 */
		static void PingPong(struct sockaddr *addr, socklen_t addrlen, uint frequency_us = 100'000) {
			static PegboardState pegboard[MAX_PEGBOARD];      	// <-- Track which packets have returned.
			static uint passed = 0u, failed = 0u;//, lost = 0u;	// <-- Log passed/corrupted/lost packets.
			static uint packet_num = 0u;						// <-- Internal packet number.

		//--- Create a thread that will recv() each pong message and check it off (recv'ed, corrupted on [client|server] end)
			pthread_create(&tid, &mThreadAttributes,
				[](void *) -> void* {
					int sd = socket(AF_INET, SOCK_DGRAM, 0);
					byte buffer[256];
					PingPacket *packet = reinterpret_cast<PingPacket*>(buffer);

					char peer_addr[100];		// <-- We honestly *do* know the max size (sizeof(IPv6)), but it's a good idea to assume not.
					socklen_t peer_addr_len;	// <-- With the message, we get the "from" (or sender) address.
					while (true) {
						recvfrom(sd, buffer, sizeof(buffer), 0, (struct sockaddr*)(peer_addr), &peer_addr_len); // <-- Block on recv()
						pegboard[packet->packet_num] = eReceived_fail;			// <-- Assume packet failed.

					//--- Verify the packet integrity
						if ( packet->state == eReceived_okay ) {
							int packet_crc = packet->checksum;					// <-- Save the server's checksum.
							packet->checksum = 0;								// <-- Zero it before doing the checksum.
							if ( Checksum(packet, packet_crc) == packet_crc ) {	// <-- If checksums are equal... PASS!
								passed++;										// <-- ...
								pegboard[packet->packet_num] = eReceived_okay;	//		... PASSED!
							} else {											// <-- If not, ...
								failed++;										//		... FAIL!
							}

						} else {											// <-- If not,
							failed++;										// 			... FAIL!
						}

//TODO: statistics: round trip time
					}
					return nullptr;
				},
				nullptr);

		//--- Create packet and send.
			int sd = socket(AF_INET, SOCK_DGRAM, 0);
			do {
				byte buffer[256];
				PingPacket *packet = reinterpret_cast<PingPacket*>(buffer);

			//--- Initialize
				packet->checksum = 0;
				packet->packet_num = packet_num++;
				packet->timestamp = time(nullptr);
				packet->packet_size = 256;
				packet->state = eSent;
				pegboard[packet->packet_num] = eSent;

			//--- Fill with garbage
				for ( uint i = 0; i < sizeof(buffer) - sizeof(PingPacket); i++ ) {  // <-- a little old-C trick: since buffer[] was never given a size, its size is zero
					packet->buffer[i] = random() % 257;
				}

			//--- Load the checksum and mark in the pegboard
				packet->checksum = Checksum(packet, sizeof(buffer));
				if ( packet_num++ == 0 ) {
					memset(pegboard, eUndefined, sizeof(pegboard));
				}

			//--- Send
				sendto(sd, buffer, sizeof(buffer), 0, addr, addrlen);
				usleep(frequency_us);
			} while (true);
		}

/** void PongPing(struct sockaddr *addr, socklen_t addrlen, double loss_percent = 0.0, uint delay_ms = 0)
 *		Receiving client to PingPong() peer.
 *
 * __GENERAL_ALGORITHM___
 *		* Create a socket that the method will listen to
 *		* Loop while...
 *			+ wait for message
 *			+ check message integrity
 *			+ flag the message
 *			+ echo message back
 */
		static void PongPing(struct sockaddr *addr, socklen_t addrlen, double loss_percent = 0.0, uint max_delay_ms = 0) {
			int sd = socket(AF_INET, SOCK_DGRAM, 0);
			if ( sd > 0 ) {
				char buffer[256];
				socklen_t peer_addr_len;
				PingPacket *packet = reinterpret_cast<PingPacket*>(buffer);

			//--- Loop on getting and echoing back messages from peer
				do {
					char peer_addr[100];
					uint bytes = recvfrom(sd, buffer, sizeof(buffer), 0, (struct sockaddr*)(&peer_addr), &peer_addr_len);
					int packet_crc = packet->checksum;									// <-- save checksum field
					packet->checksum = 0;												// <-- Clear checksum field
					bool passed = (Checksum(packet, packet_crc) == packet_crc);			// <-- Verify...
					passed = passed && (bytes == packet->packet_size);					//				integrity
  					packet->state = (passed? eReceived_okay: eReceived_fail);			// <-- Report integrity
					usleep(random() % max_delay_ms);									// <-- Simulate propagation time
					sendto(sd, buffer, bytes, 0, (struct sockaddr*)(&peer_addr), peer_addr_len);
				} while (true);

			} else {
				std::cout << "!!!" << __FUNCTION__ << "[Line# " << __LINE__ << "]: Failed to create a datagram socket!" << std::endl;
				abort();
			}

		}
};

/**********************************************************************************************
 * String GenerateHttpReport(std::vector<String>& report_log)
 *
 * Purpose:
 * 		This method generates the server-ping log page.
 */
String GenerateHttpReport(std::vector<String>& report_log) {
	const uint BYTES = 100;
	FILE *pp = popen("date", "r");
	char *tmps = reinterpret_cast<char*>(alloca(BYTES));
	fgets(tmps, sizeof(BYTES), pp);
	pclose(pp);
	String date_str(tmps);
	String http_reply;
	http_reply = "HTTP/1.1 200 OK\n";
	http_reply += "Date: " + date_str;
	http_reply += "Server: Lone Wolf\n";
	http_reply += "Content-Length: %-8d\n";
	http_reply += "Connection: keep-alive\n";
	http_reply += "Content-Type: text/html;charset=ISO-8859-1\n";
	http_reply += "\n";
	http_reply += mClientHtmlPage;
	for ( String logline : report_log ) {
		http_reply += logline + "<br>";
	}
	http_reply += "<hr></body>\n</html>\n\n";
	uint report_size = http_reply.GetLength() + 5;
	tmps = reinterpret_cast<char*>(alloca(report_size));
	snprintf(tmps, report_size, http_reply.GetText(), report_size);
	VERBOSE(eMinimalVerbosity, { std::cout << "Reply: " << tmps; });
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
	VERBOSE(eMinimalVerbosity, { std::cout << "Request: " << request; });

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
void* ReportDispatchServer(void*) {
	int server_sd;
	if ( (server_sd = socket(AF_INET, SOCK_STREAM, 0)) > 0 ) {
		socklen_t value = 1;
		if ( setsockopt(server_sd, SOL_SOCKET, SO_REUSEPORT, &value, sizeof(value)) == 0 ) {   // <-- Ensure that the port is "unlocked."

			struct sockaddr_in addr = { .sin_family = AF_INET, .sin_port = htons(mServerPort) };
			addr.sin_addr.s_addr = INADDR_ANY;
			if ( bind(server_sd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == 0 ) {   // <-- Assign port

				if ( listen(server_sd, 5) == 0 ) {
					socklen_t client_addr_size;

					while (mRun) {
						int client_sd = accept(server_sd, reinterpret_cast<struct sockaddr*>(&addr), &client_addr_size);

						VERBOSE(eNoVerbosity, { std::cout << "Client connection: " << std::endl; });//FIXME: get reentrant address

						pthread_create(&tid, &mThreadAttributes, ReportServlet, new SocketStreamChannel(client_sd));
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
 * void *PingService(void *arg) {Follows pthread's thread interface}
 * Purpose:
 * 		The thread
 * 			> attempts to connect to external server,
 * 			> measures the time it takes in milliseconds,
 * 			> generates a report,
 * 			> then closes the connection.
 *
 * @param void *arg - generic reference to AddressInfo.
 * @returns host    - which is ignored.
 */
#define RETRIES 20
void *PingService(AddressInfo *host) {
	VERBOSE(eMinimalVerbosity, { std::cout << "Open socket (family=" << host->family << " socket-type=" << host->socktype << ")" << std::endl; });

//--- Create TCP/IP socket
	int sd = socket(host->family, host->socktype, 0);
	if ( sd > 0 ) {
		VERBOSE(eNoVerbosity, { std::cout << host; });

	//--- Attempt connection
		std::cout << "Connecting to " << host->canonname << "... ";
		int retries = RETRIES;
		while ( --retries >= 0 ) {

		//--- Attempt to connect
			if ( connect(sd, host->addr, host->address_len) == 0 ) {

			//--- Attempt to send() a few bytes... if error, then true connection failed.
				std::cout << "verify " << host->canonname << "'s connection with simple send()";
				if ( send(sd, "test", 4, 0) < 0 ) {
					perror("send()");

				} else {
					break;
				}
			}
			std::cout << ". ";
			usleep(1000);
		}
		std::cout << std::endl;

	//--- Report string
		String log_entry = "\t" + host->canonname + (retries <= 0? " failure!": " success! (" + String(RETRIES - retries) + "us)");
		std::cout << log_entry << std::endl;
		pthread_mutex_lock(&mReportLog_mutex);
		mReportLog.push_back(log_entry);
		pthread_mutex_unlock(&mReportLog_mutex);

	//--- Shutdown
		std::cout.flush();
		shutdown(sd, SHUT_WR);
		close(sd);

	} else {
		perror("socket()");
	}
	return host;
}

/**********************************************************************************************
 * void ProcessCommandLine(char **args)
 *
 * Purpose:
 * 		This method processes the arguments passed in from the command line.
 *
 * @param char **args - the command line arguments.
 */
void ProcessCommandLine(char **args) {
	const String appname(*args);
	while ( *++args != nullptr ) {
		String string(*args);
		if ( string.StartsWith("--port=") ) {
			mServent = String(*args, strlen("--port="));

		} else if ( string == "--IPv4" ) {
			mFamily = AF_INET;

		} else if ( string == "--IPv6" ) {
			mFamily = AF_INET6;

		} else if ( string == "--ICMP" ) {
			std::cout << "ICMP is not yet implemented.";
			mFamily = AF_PACKET;
			mSocketType = SOCK_RAW;

		} else if ( string == "--UDP=client" ) {
			mSocketType = SOCK_DGRAM;
			mUDP_Client = true;

		} else if ( string == "--UDP=server" ) {
			mSocketType = SOCK_DGRAM;
			mUDP_Client = false;

		} else if ( string.StartsWith("--verbose=none") ) {
			mVerboseLevel = eNoVerbosity;

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

		} else if ( string == "--run-tests") {
			unit_tests();
			exit(0);

		} else if ( string == "--help" ) {
			std::cout << appname <<
					"\t[--port=[[-a-z0-9._/]+|[0-9]+]] - what port to ping (default=80/HTTP)" << std::endl <<
					"\t[--IPv4|--IPv6] - use IPv4 or IPv6 (default=IPv4)" << std::endl <<
					"\t[--UDP=[client|server] - {disabled}" << std::endl <<
					"\t[--verbose=[min|max]] - what kind of verbosity (default=none)" << std::endl <<
					"\t[--http-port=[0-9]+] - what is the HTTP port (default=8080)" << std::endl <<
					"\t[--runtime=[0-9]+] - how long to run (default=10s)" << std::endl <<
					"\t[--interval=[0-9]+] - how often to ping (default=0 {once})" << std::endl <<
					"\t[--run-tests] - run unit tests" << std::endl <<
					"\t[<hostname>|<hostip]" << std::endl;
			exit(0);

		} else {
			if ( string.StartsWith("--") ) {
				std::cout << "Don't know what to do with \"" << string << "\"" << std::endl;
				abort();
			}
			mHostnames.push_back(string);
		}
	}

//--- Report elected configuration
	std::cout <<
			"Configuration:"      << std::endl <<
			"\tVerbosity="        << (mVerboseLevel == eNoVerbosity? "none": (mVerboseLevel == eMinimalVerbosity? "min": "max")) << std::endl <<
			"\tHTTP Server port=" << mServerPort << std::endl <<
			"\tPing port="        << mServent << std::endl <<
			"\tSocket family="    << mSocketFamilyNames[mFamily] << std::endl <<
			"\tSocket type="      << mSocketTypeNames[mSocketType] << std::endl <<
			"\tRun time="         << mRuntime << std::endl <<
			"\tPing interval="    << mInterval << "ms\n" <<
			"\tUDP peers="        << (mUDP_Client? "yes": "no") << std::endl;
	for ( String hostname : mHostnames ) {
		std::cout << hostname << " ";
	}
	std::cout << std::endl;
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
	pthread_attr_setdetachstate(&mThreadAttributes, PTHREAD_CREATE_DETACHED);

//--- interpret command line args
	ProcessCommandLine(args);

//--- Start HTTP server
	pthread_create(&tid, &mThreadAttributes, ReportDispatchServer, nullptr);

//--- Collect hosts' addresses
	static std::vector<AddressInfo> host_addrs = SocketStreamChannel::CollectHosts(mHostnames, mFamily, mSocketType);

//TODO: insert UDP datagram here.
//--- Run pings
	if ( host_addrs.size() > 0 ) {

	//--- For all hosts...
		for ( AddressInfo host_addr : host_addrs ) {

			pthread_create(&tid, &mThreadAttributes,
				[](void *arg) -> void* {
					AddressInfo *host = reinterpret_cast<AddressInfo*>(arg);

					VERBOSE(eNoVerbosity, { std::cout << "!!!Ping host: " << host->canonname << std::endl; });

					do {
						VERBOSE(eNoVerbosity, { std::cout << "Connecting to... " << host->canonname << std::endl; });
						PingService(host);
						usleep(mInterval * 1000);
					} while ( mInterval > 0 ); // <-- essentially, if mInterval == 0, loop once. Hokey, I know.

					mRun = false;
					delete host;
					return nullptr;
				},
				new AddressInfo(host_addr));
		}
	}

	while ( mRuntime-- > 0  &&  mRun ) {
		sleep(1);
	}
	mRun = false;
	return 0;
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

const char *mSocketTypeNames[SOCK_PACKET+1] = {
	"(null)", "Stream", "Datagram", "Raw", "Sequenced, reliable, connection-based, datagrams",
	"Sequenced, reliable, connection-based, datagrams", "Datagram Congestion Control", "(null)",
	"(null)", "(null)", "Packet"
};
const char *mSocketFamilyNames[PF_MAX] = {
	"Unspecified (0)", "Unix pipe (1)", "IPv4 (2)", "AX25 (3)", "IPX (4)", "Appletalk (5)", "NetRom (6)",
	"Bridge (7)", "ATMPVC (8)", "X25 (9)", "IPv6", "ROSE (11)", "DECnet (12)", "NETBEUI (13)", "Security (14)",
	"Key (15)", "Netlink (16)", "Packet (17)", "ASH (18)", "ECONET (19)", "ATMSVC (20)", "RDS (21)", "SNA (22)",
	"IRDA (23)", "PPPOX (24)", "WanPipe (25)", "LLC (26)", "IB (27)", "MPLS (28)", "CAN (29)", "TIPC (30)",
	"Bluetooth (31)", "IUCV (32)", "RXRPC (33)", "ISDN (34)", "PHONET (35)", "IEEE802154 (36)", "CAIF (37)",
	"ALG (38)", "NFC (39)", "VSOCK (40)", "KCM (41)", "QIPCRTR (42)", "SMC (43)", "XDP (44)", "MCTP (45)"
};
const char *mClientHtmlPage =
	"<!DOCTYPE html>\n"
	"<html lang=\"en\">\n"
	"<head>\n"
	"    <meta charset=\"UTF-8\">\n"
	"    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
	"	 <meta http-equiv=\"refresh\" content=\"2\">"
	"    <title>ActionTarget WebSocket Message Collector</title>\n"
	"    <style>\n"
	"        body { font-family: Arial, sans-serif; margin: 20px; }\n"
	"        #messages { border: 1px solid #ccc; padding: 10px; min-height: 200px; overflow-y: scroll; margin-bottom: 10px; }\n"
	"        .message { margin-bottom: 5px; padding: 5px; background-color: #f0f0f0; border-radius: 3px; }\n"
	"    </style>\n"
	"</head>\n"
	"<body>\n"
	"    <h1>WebSocket Message Collector</h1>\n"
	"    <div id=\"messages\"></div>\n"
	"    <input type=\"text\" id=\"messageInput\" placeholder=\"Type your message...\">\n"
	"    <button id=\"sendButton\">Send</button>\n"
	"\n"
	"    <script>\n"
	"        const messagesDiv = document.getElementById('messages');\n"
	"        const messageInput = document.getElementById('messageInput');\n"
	"        const sendButton = document.getElementById('sendButton');\n"
	"\n"
	"        // Establish WebSocket connection\n"
	"        const socket = new WebSocket('ws://localhost:8080'); // Replace with your server address\n"
	"\n"
	"        // Event listener for when the connection is opened\n"
	"        socket.onopen = (event) => {\n"
	"            console.log('WebSocket connection opened:', event);\n"
	"            addMessage('System: Connected to WebSocket server.');\n"
	"        };\n"
	"\n"
	"        // Event listener for incoming messages\n"
	"        socket.onmessage = (event) => {\n"
	"            console.log('Message received:', event.data);\n"
	"            addMessage(`Server: ${event.data}`);\n"
	"        };\n"
	"\n"
	"        // Event listener for connection errors\n"
	"        socket.onerror = (error) => {\n"
	"            console.error('WebSocket error:', error);\n"
	"            addMessage('System: WebSocket error occurred.');\n"
	"        };\n"
	"\n"
	"        // Event listener for when the connection is closed\n"
	"        socket.onclose = (event) => {\n"
	"            console.log('WebSocket connection closed:', event);\n"
	"            addMessage('System: Disconnected from WebSocket server.');\n"
	"        };\n"
	"\n"
	"        // Function to add messages to the display\n"
	"        function addMessage(text) {\n"
	"            const messageElement = document.createElement('div');\n"
	"            messageElement.classList.add('message');\n"
	"            messageElement.textContent = text;\n"
	"            messagesDiv.appendChild(messageElement);\n"
	"            messagesDiv.scrollTop = messagesDiv.scrollHeight; // Auto-scroll to bottom\n"
	"        }\n"
	"\n"
	"        // Event listener for sending messages\n"
	"        sendButton.addEventListener('click', () => {\n"
	"            sendMessage();\n"
	"        });\n"
	"\n"
	"        messageInput.addEventListener('keypress', (event) => {\n"
	"            if (event.key === 'Enter') {\n"
	"                sendMessage();\n"
	"            }\n"
	"        });\n"
	"\n"
	"        function sendMessage() {\n"
	"            const message = messageInput.value;\n"
	"            if (message.trim() !== '') {\n"
	"                socket.send(message);\n"
	"                addMessage(`You: ${message}`);\n"
	"                messageInput.value = ''; // Clear input field\n"
	"            }\n"
	"        }\n"
	"    </script>\n";
	// "</body>\n"
	// "</html>\n";