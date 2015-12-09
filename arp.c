#include "unp.h"
#include"arp.h"
#include<stdint.h>

#define ARP_PROTO .0x5aba
#define ARP_ID 0x9049

int bind_pf_socket()
{
	int sockfd;

	sockfd = socket(PF_PACKET, SOCK_RAW, ARPPROTO);
	//we do not bind

	return sockfd;
}

int send_arp(int sockfd, struct ether_arp* eth, int if_index, char* msg, int msg_len)
{
	struct sockaddr_ll sall;
	char* packet;
	int packet_len;
	struct ether_arp* ethcpy;
	struct arphdr* arpcpy;
	char* msgcpy;

	if(eth == NULL || msg==NULL){ return -1; }
	packet_len = sizeof(struct ether_arp) + sizeof(struct arphdr)+ msg_len;
	do{ packet = malloc(packet_len); } while(packet==NULL);
	ethcpy = (struct ether_arp *) &packet[0];
	memcpy(ethcpy, eth, sizeof(struct ether_arp));
	arpcpy = (struct arphdr *) (ethcpy + sizeof(struct ether_arp));
	msgcpy = (char*) (arpcpy + sizeof(struct arphdr));
	memcpy(msgcpy, msg, msg_len);
	
	sall.sll_family = AF_PACKET; /* Always AF_PACKET */
	sall.sll_protocol = htons(ARPPROTO); /* Physical-layer protocol */
	sall.sll_ifindex = index; /* Interface number */
	//unsigned short sll_hatype;   /* ARP hardware type */
	//unsigned char  sll_pkttype;  /* Packet type */
	sall.sll_halen = 6; /* Length of address */
	memcpy(sll_addr, eth->h_dest, 6);  /* Physical-layer*/
	
	return sendto(sockfd, packet, packet_len, 0, (struct sockaddr *)&sall, sizeof(struct sockaddr_ll));
	
}

int recv_arp(int sockfd, char* msg, int msg_len)
{
	int n;
	struct sockaddr_ll sall_rcv;	
	n = recvfrom(sockfd, msg, msg_len, 0, (struct sockaddr*) &sall_rcv, sizeof(struct sockaddr_ll);
	return n;
}

