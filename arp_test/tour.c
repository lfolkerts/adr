#include "common_defines.h"

struct gen_ll *vm_head, *pg_head;
struct tour_socket_info ts;

int main(int argc, char **argv)
{
	struct tour_pkt *pkt, rpkt;
	struct sockaddr_in dst_addr, src_addr;
    int is_multicast_set = 0;
    char buff[MAXLINE];
	char *hostname;
	int is_dest = 0, init = 0;
	struct timeval *tv = NULL;

	memset(&dst_addr, 0x0, sizeof(dst_addr));
	memset(&src_addr, 0x0, sizeof(src_addr));
	memset(&rpkt, 0x0, sizeof(rpkt));
	init_tour();	
	initialize_sockets(&ts);

	if (argc >= 2) {
        is_multicast_set = 1;
		gen_ll_init(&vm_head);
		get_vm_info(vm_head, argv, argc);
		initialize_mcast_socket(&ts);
		pkt = construct_tour_packet(vm_head);
		dst_addr.sin_family = AF_INET;
		dst_addr.sin_addr.s_addr = pkt->ip_hdr.ip_dst.s_addr;
		Sendto(ts.rt_socket, pkt, sizeof(*pkt), 0,  (void *) &dst_addr, sizeof(dst_addr));
	}
	gen_ll_init(&pg_head);

	socklen_t len = sizeof(src_addr);

    fd_set rset;

    while (1) {
        FD_ZERO(&rset);
        FD_SET(ts.rt_socket, &rset);
        FD_SET(ts.pg_socket, &rset);
        if (is_multicast_set) 
            FD_SET(ts.mcast_socket_recv, &rset);
        int max_fd = max(ts.rt_socket, ts.pg_socket);

        if (is_multicast_set)
            max_fd = max(max_fd, ts.mcast_socket_recv);
        max_fd += 1;

        _select(max_fd, &rset, NULL, NULL, tv);
        if (FD_ISSET(ts.rt_socket, &rset)) {
	        Recvfrom(ts.rt_socket, &rpkt, sizeof(rpkt), 0, (void *) &src_addr, &len );
            if (ntohs(rpkt.ip_hdr.ip_id) != TOUR_ID ) {
                printf("<tour> Ignoring pkt with incorrect ID\n ");
                continue;
            }
			/* skip duplicates here */
			if (gen_ll_lookup(pg_head, &src_addr.sin_addr, sizeof(src_addr.sin_addr)) != NULL)
				continue;

            time_t ticks = time(NULL);
            snprintf(buff, sizeof(buff), "%.24s", ctime(&ticks));
            hostname = get_hostname(&src_addr.sin_addr);
            printf("<tour>  time: %s   received source routing packet from %s\n", buff, hostname);
			if (is_multicast_set == 0) {
				is_multicast_set = 1;
				initialize_mcast_socket(&ts);
			}
			int ret;

			ret = construct_tour_pkthdr(&rpkt);
			if (ret < 0) 
				is_dest = 1;
			send_ping_request( ts.pg_socket, can_ip, &src_addr.sin_addr);
			gen_ll_add(pg_head, &src_addr.sin_addr, sizeof(src_addr.sin_addr));
			if (init == 0) {
				tv = Malloc(sizeof(*tv));
				tv->tv_sec = 1;
				tv->tv_usec = 0;
				init = 1;
			}

			if (is_dest == 1) {
				printf("<tour> Destination reached\n");
				continue;
			}
			dst_addr.sin_family = AF_INET;
			dst_addr.sin_addr.s_addr = rpkt.ip_hdr.ip_dst.s_addr;
			Sendto(ts.rt_socket, &rpkt, sizeof(rpkt), 0,  (void *) &dst_addr, sizeof(dst_addr));
        } else if (FD_ISSET(ts.pg_socket, &rset)) {
            ;
        } else if (FD_ISSET(ts.mcast_socket_recv, &rset)) {
            ;
        } else {
			struct gen_ll *node = pg_head->next;

			while (node != pg_head) {
				printf("Sending ping request\n");
				send_ping_request(ts.pg_socket, can_ip, node->data);
				node = node->next;
			}
			assert(tv != NULL);
			tv->tv_sec = 1;
			tv->tv_usec = 0;
		}
    
    }
	return 0;
}
