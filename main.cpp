/*
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
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <iostream>
#include <vector>

typedef unsigned int uint;

class String {
	char *text = nullptr;
	uint length = 0;
	public:
		static constexpr char *EMPTY = const_cast<char*>("");

	public:
		String(const char *str = nullptr, uint start = 0, uint bytes = 0) {
			if ( str != nullptr ) {
				length = strlen(str);

				if ( length == 0  ||  start >= length ) {
					length = 0;
					text = const_cast<char*>(EMPTY);

				} else {
					if ( start == 0  &&  bytes == 0 ) {
	//NOOP				len = len;
					} else if ( start == 0  &&  bytes > 0  &&  bytes < length ) {
						length = bytes;
					} else if ( start == 0  &&  bytes > 0  &&  bytes >= length ) {
	//NOOP				len = len;
					} else if ( start > 0  &&  bytes == 0 ) {
						length -= start;
					} else if ( start > 0  &&  bytes > 0  &&  bytes + start < length ) {
						length = bytes;
					} else if ( start > 0  &&  bytes > 0  &&  bytes + start >= length ) {
						length -= start;
					}

					if ( length > 0 ) {
						text = strncpy(new char[length + 1], str + start, length + 1);
					}
				}

			} else {
				text = const_cast<char*>(EMPTY);
			}
		}
		String(long long value) {
			char tmps[25];
			snprintf(tmps, sizeof(tmps), "%lld", value);
			length = strlen(tmps);
			text = new char[length + 1];
			strncpy(text, tmps, length);
		}
		String(const String& string): length(string.length) {
			text = (!string.IsEmpty()? strdup(string.text): const_cast<char*>(EMPTY));
		}
		virtual ~String(void) {
			if ( *text != 0 ) {
				delete [] text;
			}
		}

	public:
		std::vector<String> Split(char c) {
			std::vector<String> results;
			uint head = 0, tail = 0;
			while ( head < length ) {
				if ( text[head] == c ) {
					results.push_back(String(text + tail, head - tail));
				}
			}
			if ( tail < head ) {
				results.push_back(String(text + tail, head - tail));
			}
			return results;
		}

	public:
		String& operator=(const String& string) {
			if ( !IsEmpty() ) {
//FIXME: mem leak				delete [] text;
				if ( !string.IsEmpty() ) {
					text = strdup(string.text);
				}
				length = string.length;

			} else if ( !string.IsEmpty() ) {
				text = strdup(string.text);
				length = string.length;
			}
			return *this;
		}
		friend std::ostream& operator<<(std::ostream& stream, const String& string) {
			return (stream << string.GetText());
		}
		bool operator==(const String& string) {
			return (strcmp(text, string.text) == 0);
		}

	public:
		bool StartsWith(const String& target) const {
			const char *str = text;
			const char *sub = target.text;
			while ( *sub != 0  &&  *str == *sub ) {
				str++, sub++;
			}
			return (*sub == 0);
		}
		bool IsEmpty(void) const {
			return (*text == 0);
		}
		uint GetLength(void) const {
			return length;
		}
		const char* GetText(void) const {
			return text;
		}
};

#define TEST_EQUALS(a, b) 	{ \
								printf("%s[%d] %s == %s... ", __FILE__, __LINE__, #a, #b); \
								auto _a = a; \
								auto _b = b; \
								if ( _a == _b ) { \
									std::cout << "passed.\n"; \
								} else { \
									std::cout << "failed. Expected " << _a << " but got \"" << _b << "\"" << std::endl; \
								} \
							}

void unit_tests(void) {
	static const char *test_str = "This is a test";
	String string;
	TEST_EQUALS(string.GetLength(), 0u);
	TEST_EQUALS(string.GetText(), String::EMPTY);
	const char *str;
	uint offset = 0;
	uint count = 0;

//---
	string = String(test_str);
	TEST_EQUALS(string.GetLength(), strlen(test_str));
	TEST_EQUALS(string, test_str);

//---
	offset = 1u;
	string = String(test_str, offset);
	TEST_EQUALS(string.GetLength(), strlen(test_str + offset));
	TEST_EQUALS(string, test_str + offset);

	offset = 10;
	string = String(test_str, offset);
	TEST_EQUALS(string.GetLength(), strlen(test_str + offset));
	TEST_EQUALS(string, test_str + offset);

	offset = 14u;
	string = String(test_str, offset);
	TEST_EQUALS(string.GetLength(), strlen(test_str + offset));
//	TEST_EQUALS(string, test_str + offset);

	offset = 15u;
	string = String(test_str, offset);
	TEST_EQUALS(string.GetLength(), 0u);
	TEST_EQUALS(string, "");

//---
	offset = 1u;
	count = 0u;
	str = strncpy((char*)alloca(strlen(test_str + offset) + 1), test_str + offset, count + 1);
	string = String(test_str, offset, count);
	TEST_EQUALS(string.GetLength(), strlen(test_str));
	TEST_EQUALS(string, str);

	count = 1u;
	str = strncpy((char*)alloca(strlen(test_str + offset) + 1), test_str + offset, count + 1);
	string = String(test_str, offset, count);
	TEST_EQUALS(string.GetLength(), strlen(test_str));
	TEST_EQUALS(string, str);

	count = 5u;
	str = strncpy((char*)alloca(strlen(test_str + offset) + 1), test_str + offset, count + 1);
	string = String(test_str, offset, count);
	TEST_EQUALS(string.GetLength(), strlen(test_str));
	TEST_EQUALS(string, str);

	count = 13u;
	str = strncpy((char*)alloca(strlen(test_str + offset) + 1), test_str + offset, count + 1);
	string = String(test_str, offset, count);
	TEST_EQUALS(string.GetLength(), strlen(test_str));
	TEST_EQUALS(string, str);

	count = 14u;
//	str = strncpy((char*)alloca(strlen(test_str + offset) + 1), test_str + offset, count + 1);
	string = String(test_str, offset, count);
	TEST_EQUALS(string.GetLength(), 0u);
	TEST_EQUALS(string, "");

	count = 15u;
	str = strncpy((char*)alloca(strlen(test_str + offset) + 1), test_str + offset, count + 1);
	string = String(test_str, offset, count);
	TEST_EQUALS(string.GetLength(), strlen(test_str));
	TEST_EQUALS(string, "");
}

struct ThreadState {
	int sd;
	ThreadState(int sd): sd(sd) {}
	virtual ~ThreadState(void) {
		shutdown(sd, SHUT_WR);
		close(sd);
	}
};

const char *HTTP_Format_Header =
	"HTTP/1.1 200 OK\n"
	"Date: %s"
	"Server: Lone Wolf\n"
	"Content-Length: %%8d\n"
	"Connection: keep-alive\n"
	"Content-Type: text/html;charset=ISO-8859-1\n"
	"\n"
	"%s\n";

const char *HTTP_Body =
	"<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\n"
	"<html>\n"
	"<head><meta http-equiv=\"refresh\" content=\"5\">\n"
	"<title>Index of /</title>\n"
	"</head>\n"
	"<body>\n"
	"<h1>%s</h1>\n"
	"<hr>This is a test...of the emergency broadcast system\n"
	"</body>\n"
	"</html>\n"
	"\n";
	// const char *msg = "<http><head>This is a test</header><body>...of the emergency broadcast system</body></http>";

enum VerboseLevel {
	eNone, eMinimal, eMaximal
}; 

VerboseLevel verbose_level = eNone;

void *servlet(void *arg) {
	ThreadState *state = reinterpret_cast<ThreadState*>(arg);

	FILE *pp = popen("date", "r");
	char tmp_date[50];
	fgets(tmp_date, sizeof(tmp_date), pp);
	pclose(pp);

	char buf1[10*1024];
	recv(state->sd, buf1, sizeof(buf1), 0);
	if ( verbose_level == eMaximal ) {
		fprintf(stderr, "%s", buf1);
	}
	snprintf(buf1, sizeof(buf1), HTTP_Format_Header, tmp_date, HTTP_Body);
	if ( verbose_level == eMaximal ) {
		fprintf(stderr, "[%s]", buf1);
	}
	char buf2[10*1024];
	snprintf(buf2, sizeof(buf2), buf1, strlen(buf1), tmp_date);
	if ( verbose_level == eMaximal ) {
		fprintf(stderr, "[%s]", buf2);
	}
	send(state->sd, buf2, strlen(buf2), 0);
	sleep(1);
	delete state;
	return arg;
}

uint16_t server_port = 8080;

bool run = true;
void* server(void*) {
	int server_sd;
	if ( (server_sd = socket(AF_INET, SOCK_STREAM, 0)) > 0 ) {
		socklen_t value = 1;
		if ( setsockopt(server_sd, SOL_SOCKET, SO_REUSEPORT, &value, sizeof(value)) == 0 ) {
			struct sockaddr_in addr = { .sin_family = AF_INET, .sin_port = htons(server_port) };
			addr.sin_addr.s_addr = INADDR_ANY;
			if ( bind(server_sd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == 0 ) {
				if ( listen(server_sd, 5) == 0 ) {
					socklen_t client_addr_size;
					while (run) {
						int client_sd = accept(server_sd, reinterpret_cast<struct sockaddr*>(&addr), &client_addr_size);
						if ( verbose_level > eNone ) {
							std::cerr << "Client connection: " << std::endl; //FIXME: get reentrant address
						}
						pthread_t tid;
						pthread_create(&tid, nullptr, servlet, new ThreadState(client_sd));
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

struct AddressInfo {
	int flags;
	int family;		
	int socktype;		
	int protocol;		
	socklen_t addrlen;		
	// union {
		struct sockaddr* addr;
		struct sockaddr_in addr_in4;
		// struct sockaddr_in6 addr_in6;	
	// };
	String canonname;	
	AddressInfo(addrinfo *info): flags(info->ai_flags), family(info->ai_family), socktype(info->ai_socktype), 
								protocol(info->ai_protocol), addrlen(info->ai_addrlen), canonname(info->ai_canonname) 
	{
		if ( addrlen == sizeof(struct sockaddr_in) ) {
			// addr_in4 = *(struct sockaddr_in*)(info->ai_addr);
			addr = reinterpret_cast<struct sockaddr*>(memcpy(new struct sockaddr_in(), info->ai_addr, addrlen));

		} else if ( addrlen == sizeof(struct sockaddr_in6) ) {
			// addr_in6 = *(struct sockaddr_in6*)(info->ai_addr);
			addr = reinterpret_cast<struct sockaddr*>(memcpy(new struct sockaddr_in6(), info->ai_addr, addrlen));

		} else {
			// addr = *(info->ai_addr);
			addr = reinterpret_cast<struct sockaddr*>(memcpy(new struct sockaddr(), info->ai_addr, addrlen));
		}
	}
	virtual ~AddressInfo(void) {
		// delete addr;
	}
	friend std::ostream& operator<<(std::ostream& stream, const AddressInfo& addrinfo) {
		return (stream << "flags=" << addrinfo.flags << " family=" << addrinfo.family << " socktype=" << addrinfo.socktype 
				<< " protocol=" << addrinfo.protocol << " addr=" << inet_ntoa(addrinfo.addr_in4.sin_addr) << " name=" 
				<< addrinfo.canonname);
	}

};

String servent = "80";
int family = AF_UNSPEC;
uint16_t interval = 0; // seconds
std::vector<String> hostnames;

void ProcessCommandArgs(char **args) {
	while ( *++args != nullptr ) {
		String string(*args);
		if ( string.StartsWith("--port=") ) {
			servent = String(*args, 7);

		} else if ( string == "--IPv4" ) {
			family = AF_INET;

		} else if ( string == "--verbose=minimal" ) {
			verbose_level = eMinimal;

		} else if ( string == "--verbose=maximal" ) {
			verbose_level = eMaximal;

		} else if ( string.StartsWith("--http-port=") ) {
			server_port = atoi(String(*args, strlen("--http-port=")).GetText());

		} else if ( string == "--IPv6" ) {
			family = AF_INET6;

		} else if ( string.StartsWith("--interval=") ) {
			interval = atoi(*args + strlen("--interval="));

		} else if ( string == "--help" ) {
			std::cerr << "[--verbose=[minimal|maximal]] --port=[0-9]+] [--interval=[0-9]+] [--IPv4|--IPv6|] [<hostname>|<hostip]\n";
			exit(0);

		} else {
			hostnames.push_back(string);
		}
	}
}

int main(int cnt, char *args[]) {
//	unit_tests();

//--- Start HTTP server
	pthread_t tid;
	pthread_create(&tid, nullptr, server, nullptr);
	pthread_detach(tid);

//--- Initialize vars
	// String servent = "80";
	// int family = AF_UNSPEC;
	// uint16_t interval = 0; // seconds
	// std::vector<String> hosts;

//--- 
servent = "80";
family = AF_INET;
hostnames.push_back("microsoft.com");

//--- interpret command line args
	ProcessCommandArgs(args);

//--- Collect hosts' addresses
	std::vector<AddressInfo> host_addrs;
	struct addrinfo addrinfo_hint = { 
		.ai_flags = (AI_V4MAPPED | AI_ADDRCONFIG | AI_CANONIDN), 
		.ai_family = family, 
		.ai_socktype = SOCK_STREAM
	};
	struct addrinfo *addr_info;

	int getaddrinfo_err = getaddrinfo(hostnames[0].GetText(), servent.GetText(), &addrinfo_hint, &addr_info);
	if ( getaddrinfo_err == 0 ) {
		if ( verbose_level > eNone ) {
			for ( struct addrinfo *p = addr_info; p != nullptr; p = p->ai_next ) {
				sockaddr_in *addr = (sockaddr_in*)(p->ai_addr);
				char addr_txt[100];
				inet_ntop(p->ai_family, &addr, addr_txt, p->ai_addrlen);
				fprintf(stdout, "Family: %d Name: \"%s\" address: %s\n", p->ai_family, p->ai_canonname, addr_txt);
			//	std::cerr << "name: \"" << p->ai_canonname << "\" address: " << addr_txt << "\n";
			}
		}
		host_addrs.push_back(addr_info);
		freeaddrinfo(addr_info);

	} else {
		std::cerr << "Failure: getaddrinfo(" << hostnames[0] << ", " << servent << "): " << gai_strerror(getaddrinfo_err);
	}

	if ( host_addrs.size() > 0 ) {
		do {
			for ( AddressInfo host : host_addrs ) {
std::cerr << host << std::endl;
				int sd = socket(AF_INET, SOCK_STREAM, 0);
				if ( sd > 0 ) {
					// fprintf(stderr, "Connecting to %s... ", inet_ntoa(reinterpret_cast<sockaddr_in*>(host_addrs->ai_addr)->sin_addr));
					fprintf(stderr, "Connecting to %s... ", inet_ntoa(reinterpret_cast<sockaddr_in*>(host.addr)->sin_addr));
					if ( connect(sd, host.addr, host.addrlen) == 0 ) {
						fprintf(stderr, "success!\n");
					} else {
						fprintf(stderr, "failure!\n");
					}
					shutdown(sd, SHUT_WR);
					close(sd);

				} else {
					perror("socket()");
				}
			}
			sleep(interval);
		} while ( interval > 0 );
		std::cerr.flush();
	}

	sleep(100);
	run = false;
}
