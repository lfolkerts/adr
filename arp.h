#ifndef __ARP_H__
#define __ARP_H__

#include <linux/if_ether.h>
#include <stdint.h>
#define ARP_REPLY_TYPE 1
#define ARP_REQ_TYPE 2

struct arphdr
{
	struct ethhdr eth_hdr;
	struct ether_arp e;
	uint16_t id;
};

int bind_pf_socket();
int send_arp(int sockfd, struct ether_arp* eth, int if_index, char* msg, int msg_len);
int recv_arp(int sockfd, char* msg, int msg_len);
#endif
