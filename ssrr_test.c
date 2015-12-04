#define NOT_YET
#ifdef NOT_YET
#include "unp.h"
#include "hw_addrs.h"
#include "params.h"
#include <stdint.h>

#define TEST

#define MSGARG_LEN 16
#define RT_MSG_LEN 256
#define MAX_ARGS  RTMSG_LEN/MSGARG_LEN


int main(int argc, char **argv)
{
	int rtfd, max_fd=0;
	int src_port, dest_port;
	char rt_smsg[RT_MSG_LEN], rt_rmsg[RT_MSG_LEN];
	int so_d = 1;
	socklen_t salen;
	struct sockaddr	*sasend, *sarecv;

	char template[] = "/tmp/tmpfile_XXXXXX";
	char lhost_name[HOST_NAME_MAX];
	char canonical_ip_src[INET_ADDRSTRLEN];	

	fd_set rset;
	struct timeval tv;
	uint8_t 	to_nflag = 1, //timeout_bar  flag
			recv_flag = 0, //recieve flag
			me_flag = 0,   //me flag, for when node is start
			group_flag = 0; //whether the node is in group and should start pinging
#ifdef TEST	
	sleep(START_DELAY);	
	//get different random seeds
	srand(time(NULL)*getpid());
#endif
	if(argc < 2)
	{
		fprintf(stderr, "Aborting: Incorrect Usage:%s\n\t Should be something like: ./ssrr vm1 vm5 vm3...\n", argv[1]);
		exit(1);
	}
	if(argc > MAX_ARGS)
        {
                fprintf(stderr, "Aborting: Too Many Arguments:%s\n", argv[1]);
                exit(1);
        }


	do
	{
		src_port = get_eph_port();
		dest_port = get_eph_port();
	}while(src_port == SERVER_PORT && dest_port==SERVER_PORT);
	
	get_lhostname(lhost_name, HOST_NAME_MAX);
	me_flag = (strcmp(lhost_name, argv[1])==0)?1:0;

	mkstemp(template);
	unlink(template);
	rtfd = bind_icmp_socket(MCAST_ADDR, src_port, &sasend, &salen);

	sarecv = Malloc(salen);
	memcpy(sarecv, sasend, salen);

	if(me_flag) //compose first message with translate argv
	{
		for(i=1; i<argc; i++)
		{
			msgarg_ind = i-1;
			
			ip_addr = get_canonical_ip(argv[i]);
			if(ip_addr == NULL || argv[i][0]!='v' || argv[i][1] != 'm')
                        {
                                fprintf(stderr, "Aborting: Invalid vm name: %s\n\tOther machines will hang\n", argv[i]);
                                exit(1);//TODO send an error multicast message before aborting to let other nodes know we are done
                        }
			for(j=0; j<MSGARG_LEN; j++){ ipaddr_buf[j]=0; }//empty buffer
			snprintf(ipaddr_buf, MSGARG_LEN, "%s", ip_addr);
			memcpy(&rt_rmsg[msgarg_ind*MSGARG_LEN], ipaddr_buf, MSGARG_LEN);
		}
	}

	while(pg_flag==0) //no pinging yet - no need for sleep timer
	{
		/* Send */
		if(me_flag==1)
		{
			memcpy(rt_smsg, &rt_rmsg[1*MSGARG_LEN], RT_MSG_LEN-MSG_ARG_LEN);
			strncpy(send_ipaddr, &rt_rmsg[0], MSGARG_LEN); 
			send_icmp(send_ipaddr, rt_smsg);
			//add to ping list
		}
		tv = NULL;
		max_fd = (rtfd>maxfd)?(rtfd + 1):maxfd;

		FD_ZERO(&rset);
		FD_SET(rtfd, &rset);
		to_nflag = _select(max_fd, &rset, NULL, NULL, &tv);
		if (to_nflag == 0 && recv_flag == 0) //timeout
		{
			printf("Node %s: Waiting for approval\n", lhost_name);
		}
		else if(to_nflag==0 && recv_flag == 1)
		{
			/* we got here due to a timeout, send the msg again but now with the flag to force route discovery */
			printf("Node %s: Tour has ended\n", lhost_name);
			break; 
		} else if (FD_ISSET(recv_rtfd, &rset)) 
		{
			me_flag = 1;
			recv_icmp(recv_rtfd, rt_rmsg, MCAST_MSG_LEN, salen);
			printf("Node %s Recieved: %s\n", lhost_name, rt_rmsg);
		}	
	}

	close(send_rtfd);
	close(recv_rtfd);
	return 1;
}
#endif
