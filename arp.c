#include "unp.h"
#include"arp.h"
#include <linux/if_packet.h>
#include <net/if.h>

#include<stdint.h>

#define ARP_PROTO 0x5aba

int bind_pf_socket(int if_index)
{
	int sockfd;
	struct sockaddr_ll sall;
	int err;
	struct ifreq ifr;
	char interface[10];

	sockfd = socket(PF_PACKET, SOCK_RAW, ARP_PROTO);


	strncpy (interface, "eth0", 10);
	memset (&ifr, 0, sizeof(struct ifreq));
	snprintf (ifr.ifr_name, sizeof (ifr.ifr_name), "%s", interface);
	err = ioctl (sockfd, SIOCGIFINDEX, &ifr);
	if(err){ perror("Aborting: IOCTL Error\n"); exit(1); }
	err = setsockopt (sockfd, SOL_SOCKET, SO_BINDTODEVICE, &ifr, sizeof (ifr));
	if(err){ perror("Aborting: Set Sock Opt Error2\n"); exit(1); }
	
	ioctl(sockfd,SIOCGIFFLAGS,&ifr);
	ifr.ifr_flags|=IFF_PROMISC;
	ioctl(sockfd,SIOCSIFFLAGS,&ifr);
	
	memset( &sall, 0, sizeof( sall ) );
	//sall.sll_protocol = htons(ETH_P_ALL); /* Physical-layer protocol */
	sall.sll_protocol = htons(ARP_PROTO); /* Physical-layer protocol */
	sall.sll_ifindex = if_index; /* Interface number */
        sall.sll_family = AF_PACKET; /* Always AF_PACKET */
	Bind(sockfd, (struct sockaddr*) &sall, sizeof(sall));
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
	packet_len =  sizeof(struct myarphdr)+ msg_len;
	do{ packet = malloc(packet_len); } while(packet==NULL);
	arpcpy = (struct myarphdr *) (packet);
	msgcpy = (char*) (arpcpy + sizeof(struct myarphdr));
	if(msg != NULL) { memcpy(msgcpy, msg, msg_len); }

	sall.sll_family = AF_PACKET; /* Always AF_PACKET */
	sall.sll_protocol = htons(ARP_PROTO); /* Physical-layer protocol */
	sall.sll_ifindex = if_index; /* Interface number */
	sall.sll_halen = ETH_ALEN;
	memcpy(sall.sll_addr, arp->eth.ether_dhost, ETH_ALEN);  /* Physical-layer*/

	return sendto(sockfd, packet, packet_len, 0, (struct sockaddr *)&sall, sizeof(struct sockaddr_ll));

}

struct sockaddr_ll* recv_arp(int sockfd, char* msg, int msg_len)
{
	socklen_t len;
	struct sockaddr_ll* sall_rcv;
	char *ids;
	uint16_t id;

	do{ sall_rcv = malloc(sizeof(struct sockaddr_ll)); } while(sall_rcv==NULL);
	recvfrom(sockfd, msg, msg_len, 0, (struct sockaddr*) sall_rcv, &len);	
	ids = &msg[sizeof(struct myarphdr)-2];	
	memcpy(&id, ids, 2);
	if(id != htons(ARP_ID)){ return NULL;}
	return sall_rcv;
}

