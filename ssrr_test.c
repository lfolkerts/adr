#include "unp.h"
#include "hw_addrs.h"
#include "params.h"
#include "rt.h"
#include "minix.h"
#include <stdint.h>
#include<signal.h>
#include<stdio.h>
#include <stdlib.h>
#ifdef SSRR_TEST

#define TEST
#define TO_EXIT 300 //timeout if no signal was recieved for a long time
#define IPADDR_STRLEN 16
#define IPADDR_LEN 4
#define MSGARG_LEN IPADDR_LEN
#define RT_MSG_LEN 256
#define MAX_ARGS  RT_MSG_LEN/MSGARG_LEN
#define NUM_PINGS 5 

#define MCAST_MSG_LEN 512

static uint8_t ping_flag = 0;
static void ping_alarm(int signo){ ping_flag = 1;  signal(signo, ping_alarm); return; }
static void fprint_bytes(FILE* file, char* buf, int size);

int main(int argc, char **argv)
{
	int i, j, msgarg_ind; //indexes
	int rtfd, send_mcastfd, recv_mcastfd, max_fd=0;
	int src_port, dest_port;
	char rt_smsg[RT_MSG_LEN], rt_rmsg[RT_MSG_LEN], mcast_rmsg[MCAST_MSG_LEN];
	int rt_len=0; //length of the message we are sending
	int so_d = 1;
	char ip_addr[IPADDR_STRLEN], ipaddr_buf[IPADDR_STRLEN];
	uint32_t sendrt_ip4addr, destrt_ip4addr;
	struct sockaddr * recv_addr;
	char *vm_recv;
	int rt_rmsg_len;
	char template[] = "/tmp/tmpfile_XXXXXX";
	char lhost_name[HOST_NAME_MAX];
	char canonical_ip_src[INET_ADDRSTRLEN];	
	
	fd_set rset;
	struct timeval tv;
	int		timeout_err; //timeout flag/error message
	uint8_t 	recv_flag = 0, //recieve flag
			me_flag = 0,   //me flag, for when node is start
			tour_flag = 1; //whether the application is still touring
	int ping_cnt=0, ping_stop = TO_EXIT;
	time_t rawtime;
	struct tm * timeinfo;		

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
	rtfd = bind_rt_socket(src_port);
	
	if(me_flag) //compose first message with translate argv
	{
		if(me_flag){ fprintf(stderr, "Its me guys: %s\n", lhost_name); }
#ifndef TEST
		ip_addr = get_canonical_ip(lhost_name);
		for(j=0; j<MSGARG_LEN; j++){ ipaddr_buf[j]=0; }//empty bufferdd
		inet_pton(AF_INET, ip_addr, &sendrt_ip4addr);
		fprint_bytes(stderr, (char*)&sendrt_ip4addr, IPADDR_LEN);
		sendrt_ip4addr = htonl(sendrt_ip4addr);
		memcpy(ipaddr_buf, &sendrt_ip4addr, IPADDR_LEN);
		memcpy(&rt_rmsg[msgarg_ind*MSGARG_LEN], ipaddr_buf, MSGARG_LEN);
#endif
		for(i=1; i<argc; i++)
		{
#ifdef TEST		
			msgarg_ind = i-1;
#else
			msg_arg_ind = i;
#endif
			strncpy(ip_addr, get_canonical_ip(argv[i]), IPADDR_STRLEN);
			if(ip_addr == NULL || argv[i][0]!='v' || argv[i][1] != 'm')
			{
				fprintf(stderr, "Aborting: Invalid vm name: %s\n\tOther machines will hang\n", argv[i]);
				exit(0);
			}
			for(j=0; j<MSGARG_LEN; j++){ ipaddr_buf[j]=0; }//empty buffer
			inet_pton(AF_INET, ip_addr, &destrt_ip4addr);
			destrt_ip4addr = htonl(destrt_ip4addr);
			fprint_bytes(stderr, (char*)&destrt_ip4addr, IPADDR_LEN);
			memcpy(ipaddr_buf, &destrt_ip4addr, IPADDR_LEN);
			memcpy(&rt_rmsg[msgarg_ind*MSGARG_LEN], ipaddr_buf, MSGARG_LEN);
		}
		rt_len = (++msgarg_ind)*MSGARG_LEN;
		fprint_bytes(stderr, rt_rmsg, rt_len);
		me_flag=1; //still set
	}

	max_fd = (rtfd>max_fd)?(rtfd + 1):max_fd;

	signal(SIGALRM, ping_alarm);
	while(tour_flag==1) //pinging
	{
		/* Send */
		if(me_flag==1)
		{
			fprintf(stderr, "Its me again guys: %s\n", lhost_name);
			memcpy(rt_smsg, &rt_rmsg[1*MSGARG_LEN], RT_MSG_LEN-MSGARG_LEN);
			
			memcpy(&sendrt_ip4addr, &rt_rmsg[0], IPADDR_LEN); //me (from recieved buffer)
			sendrt_ip4addr = ntohl(sendrt_ip4addr);
			
			memcpy(&destrt_ip4addr, &rt_smsg[0], IPADDR_LEN); //me (from send buffer)
			destrt_ip4addr = ntohl(destrt_ip4addr); 
			vm_recv = get_vm_name(destrt_ip4addr);
			if(vm_recv != NULL){ fprintf(stdout, "Node %s sending to %s:", lhost_name, vm_recv); }
			else{ fprintf(stderr, "VM recieve node not found:"); fprint_bytes(stderr, (char*)&destrt_ip4addr, IPADDR_LEN); }
			
			rt_len -= MSGARG_LEN;
			fprint_bytes(stdout, rt_smsg, rt_len);
			send_rt(rtfd, rt_smsg, rt_len, sendrt_ip4addr, destrt_ip4addr, get_eph_port());
			me_flag = 0; //reset flag
		}


		FD_ZERO(&rset);
		FD_SET(rtfd, &rset);
		tv.tv_sec = TO_EXIT;
		if(ping_flag==1)
		{
			fprintf(stderr, "Node %s Pinging\n", lhost_name); //ping code here
			ping_flag = 0;
			ping_cnt++;
			alarm(1);
		}	

		fprintf(stderr, "Node %s Selecting\n", lhost_name);
		timeout_err = select(max_fd, &rset, NULL, NULL, &tv); //is pselect better?
		fprintf(stderr, "Node %s Select Ended\n", lhost_name);

		if (timeout_err < 0) 
		{
			fprintf(stderr, "Node %s Timeout\n", lhost_name);
			if(errno == EINTR && ping_cnt > ping_stop)
			{
					//send mcast msg to stop
					ping_flag = 0; //no more pinging
			}
			else if(errno == EINTR)
			{
				ping_flag=1;
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
		else if(FD_ISSET(rtfd, &rset))
		{ 
			fprintf(stderr, "Node %s Recieved msg\n", lhost_name);
			rt_rmsg_len = RT_MSG_LEN;
			recv_addr = recv_rt(rtfd, rt_rmsg, &rt_len, sizeof(struct sockaddr_in));
			
			time (&rawtime);
                        timeinfo = localtime(&rawtime);
			if(recv_addr==NULL) //not us
			{ 
				fprintf(stderr, "Time: %p\n\tNode %s Bad Message Recieved:",asctime(timeinfo), lhost_name); 
				fprint_bytes(stderr, rt_rmsg, rt_len);
				continue; 
			}
			vm_recv = get_vm_name((uint32_t)((struct sockaddr_in*)recv_addr)->sin_addr.s_addr);
	
			fprintf(stderr, "Time: %p\n\tNode %s Message Recieved From %s:\n", asctime(timeinfo), lhost_name, vm_recv);
			fprint_bytes(stderr, rt_rmsg, rt_len);
			if(rt_rmsg_len < MSGARG_LEN*2)//we are the last argument - stop after a couple of pings
			{
				ping_stop = ping_cnt + NUM_PINGS;
			}
			//TODO: add prev node to ping list
			ping_flag=1;//we can start pinging now
			me_flag = 1; 
		}
		/*else if(FD_ISSET(recv_mcastfd, &rset))
		{
	//		recv_rt(recv_mcastfd, mcast_rmsg, MCAST_MSG_LEN, salen);				
			printf("Node %s Recieved: %s\n", lhost_name, mcast_rmsg);
			ping_flag = 0; //no more pinging
		}	*/
	}

	//close(send_mcastfd);
	//close(recv_mcastfd);
	close(rtfd);
	return 1;
}

static void fprint_bytes(FILE* file, char* buf, int size)
{
	int i;
	for (i = 0; i < size; i++)
	{	
	    	fprintf(file,"%3d ", 0xFF & buf[i]);
		if(i%4==3) { fprintf(file," | "); }
		if(i%32==31) { fprintf(file, "\n"); }
	}
	fprintf(file, "\n");
}
#endif //NOT_YET


