#ifndef __ARP_H__
#define __ARP_H__

#include <linux/if_ether.h>
#include <netinet/ether.h>
#include <stdint.h>
#define ARP_REPLY_TYPE 1
#define ARP_REQ_TYPE 2
#define ARP_TYPE 0x0806
#define ARP_ID 0x5aba

struct myarphdr
{
	struct ether_header eth;
	struct ether_arp e;
	uint16_t id;
};

int bind_pf_socket();
int send_arp(int sockfd, struct myarphdr* arp, int if_index, char* msg, int msg_len);
struct sockaddr_ll* recv_arp(int sockfd, char* msg, int msg_len);
#endif
