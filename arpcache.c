#include"unp.h"
#include <stdint.h>
#include "arpcache.h"

#define ARP_HASH 255

struct arp_node* ARP_Arr[ARP_HASH]; //unrolled linked list for efficient cache

void arpInit()
{
	int i;
	for(i=0; i<ARP_HASH; i++)
	{
		ARP_Arr[i]=NULL;
	}
}

static int arp_hash(uint32_t ip)
{
	return ip % ARP_HASH;
}


struct arp_node* lookup_cache_entry(uint32_t ip4_addr)
{
	struct arp_node* index;
	
	index = ARP_Arr[arp_hash(ip4_addr)];

	
	while(index!=NULL)
	{
		if(index->ip4 == ip4_addr)
		{
			return index;
		}
		index = index->next;
	}

	return index;	
}

int create_cache_entry(struct arp_node* node)
{
	int int_index;
	struct arp_node* index, *check;
	if(node==NULL || node->ip4==0){return -1;}
	int_index = arp_hash(node->ip4);
	index = ARP_Arr[int_index];
	
	check = lookup_cache_entry(node->ip4);
	if(check!=NULL)
	{
		fprintf(stderr, "ARP Cache Error: tried to add a node that already exists\n");
		return -1;
	}
	//append to head of linked list
	node->next = index;
	ARP_Arr[int_index] = node;
	return 1;
	
}
void delete_cache_entry(uint32_t ip4_addr)
{
	struct arp_node* index, *index_prev=NULL;

        index = ARP_Arr[arp_hash(ip4_addr)];


        while(index!=NULL)
        {
                if(index->ip4 == ip4_addr)
                {
                        break;
                }
		index_prev = index;
                index = index->next;
        }
	if(index==NULL){ return;}
	else if(index_prev==NULL)//head of list
	{
		ARP_Arr[arp_hash(ip4_addr)] = index->next;
		free(index);
	}
	else
	{
		index_prev->next = index->next;
		free(index);
	}
	return;
}
int delete_empty_cache_entry(uint32_t ip4_addr)
{
	int fd_ret = -1;
        struct arp_node* index, *index_prev=NULL;

        index = ARP_Arr[arp_hash(ip4_addr)];


        while(index!=NULL)
        {
                if(index->ip4 == ip4_addr && index->fd != -1)
                {
			fd_ret = index->fd;
                        break;
                }
                index_prev = index;
                index = index->next;
        }
        if(index==NULL){ return -1;}
        else if(index_prev==NULL)//head of list
        {
                ARP_Arr[arp_hash(ip4_addr)] = index->next;
                free(index);
        }
        else
        {
                index_prev->next = index->next;
                free(index);
        }
        return fd_ret;
}


