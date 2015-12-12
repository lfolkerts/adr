#include "common_defines.h"



int main()
{
	int listenfd, connfd;
	struct ll_info *ll;
	struct sockaddr_un cli_addr;
	struct arp_cache_entry *c_entry = NULL;
	int max_fd;
	struct sockaddr_ll src_addr;
	char ip[INET_ADDRSTRLEN];
	uint8_t *mac = NULL;
	int i;
	socklen_t len = sizeof(cli_addr);

	init_arp_cache(NUM_CACHE_ENTRIES);
	init_tour();

	memset(&src_addr, 0x0, sizeof(src_addr));
	memset(&cli_addr, 0x0, sizeof(cli_addr));

	listenfd = bind_domain_socket(ARP_PATH);
	ll = bind_link_socket(if_head, TOUR_PROTOID);
	while (1) {
		fd_set rset;

		FD_ZERO(&rset);
		FD_SET(listenfd, &rset);
		FD_SET(ll->sockfd, &rset);		
		max_fd = max(listenfd, ll->sockfd) + 1;
		printf("Waiting in select()\n");
		_select(max_fd, &rset, NULL, NULL, NULL);
		if (FD_ISSET(listenfd, &rset)) {
			printf("got a request on the domain socket\n");
			connfd = Accept(listenfd, (void *) &cli_addr, &len);
			Read(connfd, ip, sizeof(ip));
			c_entry = lookup_arp_cache(ip);
			if (c_entry == NULL) {
				printf("no entry for %s in arp cache\n", ip);
				mac = handle_arp_request(ip, ll);
				printf("<arp> mac addr of %s :\n", ip);
				for (i = 0; i < IF_HADDR; i++) 
					printf("%02x:", mac[i]);
				printf("\n");
				Write(connfd, mac, IF_HADDR);
				close(connfd);

			} else {
				mac = c_entry->mac_addr;
				Write(connfd, mac, IF_HADDR);
				close(connfd);
			}	
		} else if (FD_ISSET(ll->sockfd, &rset)) {
			printf("got a request from the PF_PACEKT socket\n");
			socklen_t len = sizeof(struct sockaddr_ll);
			struct ethernet_frame frame;
			memset(&frame, 0x0, sizeof(frame));
			Recvfrom(ll->sockfd, &frame, sizeof(frame), 0, (void *) &src_addr, &len);
			handle_frame(&frame, ll);
		}
	}
}
