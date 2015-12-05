#define NOT_YET
#ifndef NOT_YET
#include "unp.h"
#include "hw_addrs.h"
#include "params.h"
#include <stdint.h>

#define TEST
#define TO_EXIT 300 //timeout if no signal was recieved for a long time
#define MSGARG_LEN 16
#define RT_MSG_LEN 256
#define MAX_ARGS  RTMSG_LEN/MSGARG_LEN
#define NUM_PINGS 5 

static uint8_t ping_flag = 0;
static void ping_alarm(int signo){ ping_flag = 1; return; }


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
		exit(0);
	}
	if(argc > MAX_ARGS)
	{
		fprintf(stderr, "Aborting: Too Many Arguments:%s\n", argv[1]);
		exit(0);
	}


	do
	{
		src_port = get_eph_port();
		dest_port = get_eph_port();
	}while(src_port == SERVER_PORT && dest_port==SERVER_PORT);

	get_lhostname(lhost_name, HOST_NAME_MAX);

#ifdef TEST //it is easier to test to run the same code on all machines
	me_flag = (strcmp(lhost_name, argv[1])==0)?1:0; //am I the first node listed?
#else
	me_flag =1;
#endif
	mkstemp(template);
	unlink(template);
	rtfd = bind_icmp_socket(src_port, &sasend, &salen);

	sarecv = Malloc(salen);
	memcpy(sarecv, sasend, salen);

	if(me_flag) //compose first message with translate argv
	{
#ifndef TEST
		ip_addr = get_canonical_i1p(lhost_name);
		for(j=0; j<MSGARG_LEN; j++){ ipaddr_buf[j]=0; }//empty bufferdd
		snprintf(ipaddr_buf, MSGARG_LEN, "%s", ip_addr);
		memcpy(&rt_rmsg[msgarg_ind*MSGARG_LEN], ipaddr_buf, MSGARG_LEN);
#endif
		for(i=1; i<argc; i++)
		{
#ifdef TEST		
			msgarg_ind = i-1;
#else
			msg_arg_ind = i;
#endif
			ip_addr = get_canonical_ip(argv[i]);
			if(ip_addr == NULL || argv[i][0]!='v' || argv[i][1] != 'm')
			{
				fprintf(stderr, "Aborting: Invalid vm name: %s\n\tOther machines will hang\n", argv[i]);
				exit(0);
			}
			for(j=0; j<MSGARG_LEN; j++){ ipaddr_buf[j]=0; }//empty buffer
			snprintf(ipaddr_buf, MSGARG_LEN, "%s", ip_addr);
			memcpy(&rt_rmsg[msgarg_ind*MSGARG_LEN], ipaddr_buf, MSGARG_LEN);
		}
	}

	setsockopt	
	max_fd = (rtfd>maxfd)?(rtfd + 1):maxfd;

	SIGNAL(SIGALARM, ping_alarm);
	while(tour_flag==1) //pinging
	{
		/* Send */
		if(me_flag==1)
		{
			memcpy(rt_smsg, &rt_rmsg[1*MSGARG_LEN], RT_MSG_LEN-MSG_ARG_LEN);
			strncpy(send_ipaddr, &rt_rmsg[0], MSGARG_LEN); 
			send_icmp(send_ipaddr, rt_smsg);
			me_flag = 0; //reset flag
			ping_flag = 1; //we are actively pinging
		}


		FD_ZERO(&rset);
		FD_SET(rtfd, &rset);
		tv.sec = TO_EXIT;
		if(ping_flag==1)
		{
			printf("Node %s Pinging", lhost_name);
			ping_flag = 0;
			alarm(1);
		}	
		timeout_err = _select(max_fd, &rset, NULL, NULL, &tv);

		if (timeout_err < 0) 
		{
			if(errno = EINTR); //SIGALARM interrupt as expected
			{	
				ping_cnt++;
				if(ping_cnt > ping_stop)
				{
					//send mcast msg to stop
					ping_flag = 0; //no more pinging
				}
			}
			else //unknown error
			{
				fprintf(stderr, "Abort: Node %s select error", lhost_name);
				exit(1);
			}
		}	
		else if(timeout_err == 0) //ligit timout - no packet recieved in long time abort
		{
			exit(1);
		}
		else if(FD_ISSET(recv_rtfd, &rset))
		{ 
			recv_addr = recv_icmp(recv_rtfd, rt_rmsg, RT_MSG_LEN, salen);
			vm_recv = get_vm_name(recv_addr);
			
			time (&rawtime);
			timeinfo = localtime (&rawtime);
			printf("Time: %p\n\tNode %s Recieved: %s\n", asctime(timeinfo), lhost_name, mcast_rmsg);
			if(mcast_rmsg < MSGARG_LEN*2)//we are the last argument - stop after a couple of pings
			{
				ping_stop = ping_cnt + NUM_PINGS;
			}
			//TODO: add prev node to ping list
			me_flag = 1; 
		}
		else if(FD_ISSET(recv_mcastfd, &rset))
		{
			mcast_recv(recv_mcastfd, mcast_rmsg, MCAST_MSG_LEN, salen);				
			printf("Node %s Recieved: %s\n", lhost_name, mcast_rmsg);
			ping_flag = 0; //no more pinging
		}	
	}

	close(send_rtfd);
	close(recv_rtfd);
	return 1;
}`
static void ping_alarm(int signo)
{
	ping_flag = 1;
}

#endif //NOT_YET
