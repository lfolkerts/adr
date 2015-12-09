#ifndef __ARPCACHE_H__
#define __ARPCACHE_H__

#include <stdint.h>

struct arp_node
{
        uint32_t ip4;
        struct hwa_info hwa;
        int fd;
	int sll_ifindex;
	uint16_t sll_hatype;	
};


void arpInit();
struct arp_node* lookup_cache_entry(uint32_t ip4_addr);
int create_cache_entry(arp_node* node);
void delete_cache_entry(uint32_t ip4_addr);

#endif
