#ifndef __ARPCACHE_H__
#define __ARPCACHE_H__

#include <stdint.h>
#define HWA_LEN 14


struct arp_node
{
        uint32_t ip4;
        char hwa[HWA_LEN];
        int fd;
	int sll_ifindex;
	uint16_t sll_hatype;
	struct arp_node *next;
};


void arpInit();
struct arp_node* lookup_cache_entry(uint32_t ip4_addr);
int create_cache_entry(struct arp_node* node);
void delete_cache_entry(uint32_t ip4_addr);
int delete_empty_cache_entry(uint32_t ip4_addr);

#endif
