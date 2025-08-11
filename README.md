# Action-Target Ping
*To compile and install*
1) Clone this repo.
2) Type: "make all".
3) Type: "./bin/actiontarget_ping --interval=1000 hp.com microsoft.com lwn.net office.com zilog.com --verbose=max --IPv4 --port=http"
   
*To install in systemd: (You will need root privileges to install!)*
1) Type: "make all install"
2) Restart systemd.


This project was an assignment from Action Target. *The requirements are:*
* ☑ Write a program that runs on a linux system.
* ☑ Command line flags "Hosts", "Port", and "Interval".
* ☑ Ping each host often enough to detect if they are down or up.
* ☑ Determine if a host is online or offline.
* ☐ Keep track of and display relevant metrics (latency and packet loss, etc.).
* ☐ The results should be displayed with real-time updates on a webpage.

The design entailed:
* Using C++ as the basic workhorse.
* Translation of hosts to IPv4 and IPv6 addresses using getaddrinfo(), the preferred method of getting host information.
* With TCP connections, connecting to a specified port was the only a test that TCP could handle. So, no metrics other than pass/fail could be made.
* Reports would then just show whether the connection was successful.
* Using C++ presented a problem with WebSockets which are needed for real time HTML data update.
* Later, two JavaScript scripts (a TCP/IP-socket-to-WebSocket mapper and a client) were added for the purpose of adding WebSockets.

Diagram:

[Servers...] <=> [C++ program] => [TCP/IP-socket-to-WebSocket mapper] => [HTML client]

The benefit of this approach is gaining the power and performance of C++ over the utility of a script, while keeping the scripts for a frontent to the process.

Please note that the whole system is incomplete at this time.

I chose to write this program in C++. It is by far the most powerful and demonstrative form one could use to fulfill the assignment. Despite the limitations and my lack of knowledge in more modern languages, I nonetheless chose to proceed with a C/C++ implementation. If I had the time, I could have learned the necessary particulars in other languages like JavaScript, Ruby on Rails, Go, etc. Nevertheless, this is, afterall, an evaluation of my programming skills.

There are conflicting requirements. Unless this task was to use UDP, tracking latency and packet loss is meaningless, because TCP is lossless and latency is often meaningless -- especially if the hosts are of "big-named" companies. UDP is also problematic, since unless one writes a UDP listener, there's no one out on the net with which one can test the program. I do measure the amount of time to get a reply and display it in a console window. I still have a little time, I could expand the program with UDP support.

*Features I implemented:*
* verbosity - the user can get a look at the underpinnings of the program.
* ports - named or numbered.
* interval - the ping frequency with 1ms being the smallest. Zero means once.
* unit tests - a modicum set of unit tests.
* runtime - number of seconds the program is to run.
* host list - any number of hosts to test.
* systemd - can be installed as a systemd service.
* TCP/UDP - connection can use either SOCK_STREAM or connection-based SOCK_DGRAM.
* IPv4/IPv6 - ping with IPv4 or IPv6.
* ICMP - low-level ping [TBD].
