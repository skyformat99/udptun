#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include "socketwrapper.h"

/* cxx */
#include <iostream>

using namespace std;

/* 3rd party libs */
#include <tins/tins.h>

using namespace Tins;

void process_arguments(int argc, char **argv) {
	int opt;
	
	while (~(opt = getopt(argc, argv, "hH"))) { 
		switch(opt) {
			case 'h': case 'H':
				fputs("?\n", stdout);
				exit(0);
				//break;
		}
    }
} 

void mainloop(int socketfd, SA* pcliaddr, socklen_t clilen) {
	int n;
	socklen_t len;
	char buf[MAXLINE+1];
	
	for(;;) {
		len = clilen;
		n = recvfrom(socketfd, buf, MAXLINE, 0, pcliaddr, &len);

		printf("Read %d bytes from udp socket\n", n);
		//hex_dump(buf, n);
		RawPDU p((uint8_t *)buf, n);
		try {
			IP ip(p.to<IP>());
			cout << "IP Packet: " << ip.src_addr() << " -> " << ip.dst_addr() << std::endl;
			Tins::IPv4Address srcaddr = ip.src_addr();
			ip.src_addr(ip.dst_addr());
			ip.dst_addr(srcaddr);
			ICMP &icmp = ip.rfind_pdu<ICMP>();
			icmp.type(ICMP::ECHO_REPLY);
			sendto(socketfd, ip.serialize().data(), ip.serialize().size(), 0, pcliaddr, len);
		} catch (...) {
			continue;
		}
	}
}

int main(int argc, char** argv) {

	int listenfd, connfd;
	pid_t childpid;
	socklen_t clilen;
	struct sockaddr_in cliaddr, srvaddr;
	
	process_arguments(argc, argv);
	
	listenfd = Socket(AF_INET, SOCK_DGRAM, 0);
	
	bzero(&srvaddr, sizeof(srvaddr));
	srvaddr.sin_family = AF_INET;
	srvaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	srvaddr.sin_port = htons(SERV_PORT);
	
	bind(listenfd, (SA*) &srvaddr, sizeof(srvaddr));
	
	fputs("Server now running in UDP mode.\n", stdout);
	mainloop(listenfd, (SA *) &cliaddr, sizeof(cliaddr));
}
