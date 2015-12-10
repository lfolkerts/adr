#include "unp.h"
#include"arp.h"
#include<stdint.h>

#define ARP_PROTO 0x5aba

int bind_pf_socket()
{
	int sockfd;

	sockfd = socket(PF_PACKET, SOCK_RAW, ARPPROTO);
	//we do not bind

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
	sall.sll_ifindex = index; /* Interface number */
	//unsigned short sll_hatype;   /* ARP hardware type */
	//unsigned char  sll_pkttype;  /* Packet type */
	sall.sll_halen = 6; /* Length of address */
	memcpy(sll_addr, arp->eth.ether_dhost, 6);  /* Physical-layer*/
	
	return sendto(sockfd, packet, packet_len, 0, (struct sockaddr *)&sall, sizeof(struct sockaddr_ll));
	
}

struct sockaddr_ll* recv_arp(int sockfd, char* msg, int msg_len)
{
	int n;
	struct sockaddr_ll* sall_rcv;	
	do{ sall_rcv = malloc(sizeof(struct sockaddr_ll); } while(sall_rcv==NULL);
	n = recvfrom(sockfd, msg, msg_len, 0, (struct sockaddr*) sall_rcv, sizeof(struct sockaddr_ll);	
	return sall_rcv;
}

