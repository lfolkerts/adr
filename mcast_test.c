#include "unp.h"
#include "mcast.h"
#include "hw_addrs.h"
#include "params.h"
#include "minix.h"
#define MCAST_TEST
#ifdef MCAST_TEST
#define MCAST_MSG_LEN 256
#define TOUR_END_MSG "<<<This is node %s. Tour has ended. Group members please identify yourselves>>>"
#define TOUR_END_MIDENTIFY "Tour has ended. Group members please identify yourselves"
#define IDENTIFY_MSG "<<<This is node %s: I am a part of the group>>>"
#define TOUR_TIMEOUT 1000 //in msecs
#define TEST
#ifdef TEST
 #define QUIT_PROB 0.5
 #define ENDTOUR_PROB 0.01
#endif

int main(int argc, char **argv)
{
	int send_mcastfd, recv_mcastfd, max_fd=0;
	int src_port, dest_port;
	char smsg[MCAST_MSG_LEN], rmsg[MCAST_MSG_LEN];
	int so_d = 1;
	socklen_t salen;
	struct sockaddr	*sasend, *sarecv;

	char template[] = "/tmp/tmpfile_XXXXXX";
	char lhost_name[HOST_NAME_MAX];
	char canonical_ip_src[INET_ADDRSTRLEN];	

	fd_set rset;
	struct timeval tv;
	int to_nflag = 1, recv_flag = 0; //timeout bar and recieve flags

#ifdef TEST	
	sleep(START_DELAY);	
#endif
	do
	{
		src_port = get_eph_port();
		dest_port = get_eph_port();
	}while(src_port == SERVER_PORT && dest_port==SERVER_PORT);
	
	get_lhostname(lhost_name, HOST_NAME_MAX);

#ifdef TEST	
	//get different random seeds
	srand(time(NULL)*getpid());
	if(rand()%1000<QUIT_PROB*1000)
	{
		printf("Node %s: Not Joining your group\n", lhost_name);
		exit(1);
	}
#endif

	mkstemp(template);
	unlink(template);
	send_mcastfd = bind_mcast_socket(MCAST_ADDR, src_port, &sasend, &salen);

	sarecv = Malloc(salen);
	memcpy(sarecv, sasend, salen);
	recv_mcastfd = Socket(sasend->sa_family, SOCK_DGRAM, 0);
	Bind(recv_mcastfd, sarecv, salen);

	Setsockopt(recv_mcastfd, SOL_SOCKET, SO_REUSEADDR, &so_d, sizeof(so_d));

	Mcast_join(recv_mcastfd, sasend, salen, NULL, 0);
	Mcast_set_loop(send_mcastfd, 0);
	while(1)
	{
#ifdef TEST //have a random node at a random time send out a termination message
		if(rand()%1000 < ENDTOUR_PROB*1000 && recv_flag==0)
		{
			snprintf(smsg, MCAST_MSG_LEN, TOUR_END_MSG, lhost_name);
			printf("Node %s Sending: %s\n", lhost_name, smsg);
			send_mcast(send_mcastfd, smsg, sasend, salen);	/* parent -> sends */
		}
#endif
		tv.tv_sec = TOUR_TIMEOUT/1000;
		tv.tv_usec = (TOUR_TIMEOUT%1000)*1000;	
		max_fd = recv_mcastfd + 1;

		FD_ZERO(&rset);
		FD_SET(recv_mcastfd, &rset);
		to_nflag = _select(max_fd, &rset, NULL, NULL, &tv);
		if (to_nflag == 0 && recv_flag == 0) //tim
		{
			printf("Node %s: Waiting for approval\n", lhost_name);
		}
		else if(to_nflag==0 && recv_flag == 1)
		{
			/* we got here due to a timeout, send the msg again but now with the flag to force route discovery */
			printf("Node %s: Tour has ended\n", lhost_name);
			break; 
		} else if (FD_ISSET(recv_mcastfd, &rset)) 
		{
			recv_flag = 1;
			recv_mcast(recv_mcastfd, rmsg, MCAST_MSG_LEN, salen);
			printf("Node %s Recieved: %s\n", lhost_name, rmsg);
			
			if(strstr(rmsg, TOUR_END_MIDENTIFY) != NULL) 
			{
				snprintf(smsg, MCAST_MSG_LEN, IDENTIFY_MSG, lhost_name);
				printf("Node %s Sending: %s\n", lhost_name, smsg);
				send_mcast(send_mcastfd, smsg, sasend, salen);	/* parent -> sends */
			}	
		}
	}

	close(send_mcastfd);
	close(recv_mcastfd);
	return 1;
}
#else
int main(int argc, char **argv) { return -1; }
#endif
