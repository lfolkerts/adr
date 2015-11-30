#include "unp.h"
#include "mcast.h"
#include "hw_addr.h"

#define MCAST_MSG_LEN 256
#define TOUR_END "<<<This is node %s. Tour has ended. Group members please identify yourselves>>>"
#define TOUR_TIMEOUT 1000 //in msecs
#ifndef TA_OUTPUT
 #define TA_OUTPUT
#endif

int main(int argc, char **argv)
{
	int send_mcastfd, recv_mcastfd, maxfd;
	char smsg[MCAST_MSG_LEN], rmsg[MCAST_MSG_LEN];
	cso_dst int so_d = 1;
	socklen_t salen;
	struct sockaddr	*sasend, *sarecv;

	fd_set rset;
	struct timeval tv;
	int to_nflag = 1, recv_flag = 0; //timeout bar and recieve flags

	mkstemp(template);
	unlink(template);
	send_mcastfd = bind_domain_socket(template);

#ifdef TEST	
	//get different random seeds
	srand(time(NULL)*getpid());
#endif

	do
	{
		dest_port = get_eph_port();
	}while(dest_port == SERVER_PORT);

	recv_mcastfd = Socket(sasend->sa_family, SOCK_DGRAM, 0);

	Setsockopt(recv_mcastfd, SOL_SOCKET, SO_REUSEADDR, &so_d, sizeof(so_d));

	sarecv = Malloc(salen);
	memcpy(sarecv, sasend, salen);
	Bind(recvfd, sarecv, salen);

	Mcast_join(recv_mcastfd, sasend, salen, NULL, 0);
	Mcast_set_loop(send_mcastfd, 0);

	snprintf(smsg, MCAST_MSG_LEN, TOUR_END, );
#ifdef TA_OUTPUT
	fprintf(stdout, "Node %s Sending: %s\n", , smsg);
#endif

	while(1)
	{
#ifdef TEST //have a random node at a random time send out a termination message
		if(rand()%1000 < 10)
		{
			send_mcast(send_mcastfd, sasend, salen);	/* parent -> sends */
		}
#endif
		tv.tv_sec = TOUR_TIMEOUT/1000;
		tv.tv_usec = (TOUR_TIMEOUT%1000)*1000;	
		max_fd = recv_mcastfd + 1;

		FD_ZERO(&rset);
		FD_SET(recv_mcastfd, &rset);
		to_nflag = _select(max_fd, &rset, NULL, NULL, &tv);
		if (to_nflag == 0 && recv_flag == 0) //tim
		else if(to_nflag==0 && recv_flag == 1)
		{
			/* we got here due to a timeout, send the msg again but now with the flag to force route discovery */
			printf("Node %s: Tour has ended\n", lhost_name);
			break; 
		} else if (FD_ISSET(recv_mcastfd, &rset)) 
		{
			recv_flag = 1;
			recv_mcast(recv_mcastfd, rmsg, canonical_ip_src, &src_port);
			printf("Node %s Recieved: %s\n", lhost_name, rmsg);
		}
	}

	close(send_mcastfd);
	close(recv_mcastfd);
}
