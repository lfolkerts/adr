#include "unp.h"
#include"arp.h"
#include <linux/if_packet.h>
#include <net/if.h>

#include<stdint.h>

#define ARP_PROTO 0x5aba

int bind_pf_socket(int if_index)
{
	int sockfd;
	struct sockaddr_ll addr;
	struct ifreq req;
	sockfd = socket(PF_PACKET, SOCK_RAW, ARP_PROTO);
        int err;
        struct ifreq ifr;
        char interface[10];

/*	strncpy (interface, "eth0", 10);
	memset (&ifr, 0, sizeof(struct ifreq));
	snprintf (ifr.ifr_name, sizeof (ifr.ifr_name), "%s", interface);
	err = ioctl (sockfd, SIOCGIFINDEX, &ifr);
	if(err){ perror("Aborting: IOCTL Error\n"); exit(1); }
	err = setsockopt (sockfd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof (ifr));
	if(err){ perror("Aborting: Set Sock Opt Error2\n"); exit(1); }
*/
	memset( &addr, 0, sizeof( addr ) );
	addr.sll_family   = PF_PACKET;
	addr.sll_protocol = 0;
	addr.sll_ifindex  = if_index;            
	Bind(sockfd, (struct sockaddr*) &addr, sizeof(addr));

	return sockfd;
}

int send_arp(int sockfd, struct myarphdr* arp, int if_index, char* msg, int msg_len)
{
	struct sockaddr_ll sall;
	char* packet;
	int packet_len;
	struct myarphdr* arpcpy;
	char* msgcpy;

	if(arp == NULL){ return -1; }
	packet_len = sizeof(struct ether_arp) + sizeof(struct myarphdr)+ msg_len;
	do{ packet = malloc(packet_len); } while(packet==NULL);
	arpcpy = (struct myarphdr *) (packet);
	msgcpy = (char*) (arpcpy + sizeof(struct myarphdr));
	if(msg != NULL) { memcpy(msgcpy, msg, msg_len); }

	sall.sll_family = AF_PACKET; /* Always AF_PACKET */
	sall.sll_protocol = htons(ARP_PROTO); /* Physical-layer protocol */
	sall.sll_ifindex = if_index; /* Interface number */
	//unsigned short sll_hatype;   /* ARP hardware type */
	//unsigned char  sll_pkttype;  /* Packet type */
	sall.sll_halen = 6; /* Length of address */
	memcpy(sall.sll_addr, arp->eth.ether_dhost, 6);  /* Physical-layer*/

	return sendto(sockfd, packet, packet_len, 0, (struct sockaddr *)&sall, sizeof(struct sockaddr_ll));

}

struct sockaddr_ll* recv_arp(int sockfd, char* msg, int msg_len)
{
	socklen_t len;
	struct sockaddr_ll* sall_rcv;

	do{ sall_rcv = malloc(sizeof(struct sockaddr_ll)); } while(sall_rcv==NULL);
	recvfrom(sockfd, msg, msg_len, 0, (struct sockaddr*) sall_rcv, &len);	
	return sall_rcv;
}

