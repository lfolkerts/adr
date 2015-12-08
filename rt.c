#include "unp.h"
#include "rt.h"
#include <assert.h>
#include <stdint.h>
#include <linux/ip.h>
#include<linux/icmp.h>
#include "hw_addrs.h"
#include <net/if.h>           

 #define IPHDR_ID 9049
 #define IPHDR_TTL 64
 #define IPPROTO_MYSSRR IPPROTO_ICMP //149
 #define ICMP_RTTYPE 49
 #define ICMP_RTCODE 49


#define ARRAY_SIZE(name) sizeof(name)/sizeof(name[0])
static unsigned short crc(unsigned short *addr, int len);

int bind_rt_socket(int port)
{
	struct sockaddr_in addr;
	int sockfd;
	int err;
	int on;
	struct ip *ip_hdr;
	struct icmphdr *ipproto_hdr;
	struct ifreq ifr;
	char interface[10];
	on =1;
	sockfd = Socket(AF_INET, SOCK_RAW, IPPROTO_MYSSRR);
	err = setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on));
	if(err){ perror("Aborting: Set Sock Opt Error\n"); exit(1); }
	memset(&addr, 0x0, sizeof(addr));
  	
	//Send via eth0
  	strncpy (interface, "eth0", 10);	
	memset (&ifr, 0, sizeof(struct ifreq));
 	snprintf (ifr.ifr_name, sizeof (ifr.ifr_name), "%s", interface);
  	err = ioctl (sockfd, SIOCGIFINDEX, &ifr); 
	if(err){ perror("Aborting: IOCTL Error\n"); exit(1); }
  	err = setsockopt (sockfd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof (ifr));
	if(err){ perror("Aborting: Set Sock Opt Error2\n"); exit(1); }

	addr.sin_family = AF_INET;
	addr.sin_port = port;
	addr.sin_addr.s_addr = INADDR_ANY;	
//	path = "130.245.156.20";	
//	inet_pton(AF_INET, path, &addr.sin_addr.s_addr);
//	Bind(sockfd, (struct sockaddr*) &addr, sizeof(addr));

	return sockfd;
}

struct ipprotohdr
{
#if IPPROTO_MYSSRR==IPPROTO_ICMP
	struct icmphdr icmp_hdr;
#endif
};

void send_rt(int sendfd, char* buf, int msg_len, uint32_t ip4src, uint32_t ip4dest, uint16_t dest_port)
{

	int packet_len;
	struct iphdr *ip_hdr;
	struct ipprotohdr *ipproto_hdr;
	char* packet, *bufcpy;
	struct sockaddr_in* sadest;
	socklen_t salen;	
	int i;
	// stack overflow question 13620607 helped
	do{ sadest = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));}while(sadest==NULL);
	salen = sizeof(struct sockaddr_in);
	packet_len = sizeof(struct iphdr) + sizeof(struct ipprotohdr) + msg_len;
	do{ packet = (char*) malloc(packet_len);}while(packet==NULL);
	ip_hdr = (struct iphdr*) packet;
	ipproto_hdr = (struct ipprotohdr *)(packet + sizeof(struct iphdr));
	bufcpy = (char*) (ipproto_hdr + sizeof(struct ipprotohdr));
	
	memcpy(bufcpy, buf, sizeof(buf));

	ip_hdr->ihl=5;	
	ip_hdr->tos = 4; //IPV4
	ip_hdr->version = 4;
	ip_hdr->tot_len = htons(packet_len);
	ip_hdr->id = htons(IPHDR_ID);
	ip_hdr->frag_off = htons(0); 
	ip_hdr->ttl = IPHDR_TTL;
	ip_hdr->protocol = IPPROTO_MYSSRR;
	ip_hdr->saddr = ip4src;
	ip_hdr->daddr = ip4dest;
	ip_hdr->check = htons(crc((unsigned short *)packet, sizeof(struct iphdr)));
#if IPPROTO_MYSSRR==IPPROTO_ICMP
	fprintf(stderr, "ICMP packet\n");
	ipproto_hdr->icmp_hdr.type = ICMP_RTTYPE;
        ipproto_hdr->icmp_hdr.code = ICMP_RTCODE;
        ipproto_hdr->icmp_hdr.checksum = 0;
#endif
	sadest->sin_family = AF_INET;
	sadest->sin_addr.s_addr = ip4dest;
	sadest->sin_port = dest_port;
	
	Sendto(sendfd, packet, packet_len, 0, (SA*) sadest, salen);
       	
	free(packet);
}

struct sockaddr* recv_rt(int recvfd, char* buf, int* maxsize, socklen_t salen)
{
	int n, packet_len;
	socklen_t len;
	struct iphdr *ip_hdr;
	struct ipprotohdr *ipproto_hdr;
	struct sockaddr *safrom;
	char* packet, *bufcpy;	

	packet_len = sizeof(struct iphdr) + sizeof(struct ipprotohdr) + *maxsize;
        do{ packet = (char*) malloc(packet_len);}while(packet==NULL);
        ip_hdr = (struct iphdr*) packet;
	ipproto_hdr = (struct ipprotohdr *)(packet +sizeof(struct iphdr));
        bufcpy = (char*) (ipproto_hdr + sizeof(struct ipprotohdr));

	safrom = Malloc(salen);
	len = salen;
	n = Recvfrom(recvfd, packet, packet_len, 0, safrom, &len);	
	
	if(ip_hdr->id != htons(IPHDR_ID))
	{
		fprintf(stderr, "Bad ID%X %X\n",ip_hdr->id, IPHDR_ID );
		return NULL;
	}
	
	if(1)//crc((unsigned short*)packet, n) == ip_hdr->check)
	{
		*maxsize = n-sizeof(struct iphdr)-sizeof(struct ipprotohdr);
		memcpy(buf, bufcpy, *maxsize);
	}
	else
	{
		fprintf(stderr, "Bad Checksum\n");
		return NULL;
	}
	
	free(packet);
	return safrom;
}

static unsigned short crc(unsigned short *addr, int len)
{
	return 0;
}
