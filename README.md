# Action-Target Ping

I chose to write this program in c++. It is by far the most difficult form one could use to fulfill the assignment. In fact, one feature in particular was unreachable because of the research required, and that is the implementation of real-time web updates. Despite the limitations and my lack of knowledge in more modern languages, I nonetheless chose to proceed with a c/c++ implementation. If I had the time, I could have learned the necessary particulars in other languages like JavaScript, Ruby on Rails, Go, etc. Nevertheless This is, afterall, an evaluation of my programming skills.

*The requirements are:*
* Write a program that runs on a linux system.
* Command line flags "Hosts", "Port", and "Interval".
* Ping each host often enough to detect if they are down or up.
* Determine if a host is online or offline.
* Keep track of and display relevant metrics (latency and packet loss, etc.).
* The results should be displayed with real-time updates on a webpage.

There are conflicting requirements. Unless this task was to use UDP, tracking latency and packet loss is meaningless, because TCP is lossless and latency is often meaningless -- especially if the hosts are of "big-named" companies. UDP is also problematic, since unless one writes a UDP listener, there's no one out on the net with which one can test the program. I do measure the amount of time to get a reply and display it in a console window. I still have a little time, I could expand the program with UDP support.

Unfortunately, providing a real-time update was out of my reach. So I did my best in creating a simple web server. I could have intermixed the technologies by using C++ as the workhorse and use, for example, Node.js to provide the glue for WebSockets. In the timeframe provided, that was not possible.

*Features I implemented:*
* verbosity - the user can get a look at the underpinnings of the program.
* ports - named or numbered.
* interval - the ping frequency with 1ms being the smallest. Zero means once.
* unit tests - a modicum of unit tests.
* runtime - number of seconds the program is to run.
* host list - any number of hosts to test.
* systemd - can be installed as a systemd service.
* TCP/UDP - connection can use either SOCK_STREAM or SOCK_DGRAM[TBD].
* IPv4/IPv6 - ping with IPv4 or IPv6.
* ICMP - low-level ping [TBD].

*To compile and install*
1) Clone this repo.
2) Type: "make all install" (You will need root privileges to install!)
3) Restart systemd.
