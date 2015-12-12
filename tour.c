#include "unp.h"
#include "hw_addrs.h"
#include "params.h"
#include "rt.h"
#include "minix.h"
#include "mcast.h"
#include "myping.h"
#include <stdint.h>
#include<signal.h>
#include<stdio.h>
#include <stdlib.h>

#define TO_EXIT 300 //timeout if no signal was recieved for a long time
#define IPADDR_STRLEN 16
#define IPADDR_LEN 4
#define MSGARG_LEN IPADDR_LEN
#define RT_MSG_LEN 200
#define MAX_ARGS  RT_MSG_LEN/MSGARG_LEN

#define TOUR_END_MSG "<<<This is node %s. Tour has ended. Group members please identify yourselves>>>"
#define TOUR_END_MIDENTIFY "Tour has ended. Group members please identify yourselves"
#define IDENTIFY_MSG "<<<This is node %s: I am a part of the group>>>"
#define TO_MCAST 5 //timeout after recieving an mcast message
#define NUM_PINGS 5 //number of pings before sending out mcast after final message
#define MCAST_MSG_LEN 256

static uint8_t ping_flag = 0;
static void ping_alarm(int signo){ ping_flag = 1;  signal(signo, ping_alarm); return; }
static void fprint_bytes(FILE* file, char* buf, int size);

int main(int argc, char **argv)
{
	int i, j, msgarg_ind=0; //indexes
	int rtfd=-1, send_mcastfd=-1, recv_mcastfd=-1, pingfd=-1, max_fd=0;
	int src_port, dest_port;
	char rt_smsg[RT_MSG_LEN], rt_rmsg[RT_MSG_LEN], mcast_smsg[MCAST_MSG_LEN], mcast_rmsg[MCAST_MSG_LEN];
	int repeats=0, rt_len=0; //repeated sequential nodes; length of the message we are sending
	socklen_t mcast_salen;
        struct sockaddr *mcast_sasend, *mcast_sarecv;

	int so_d = 1;
	char *ip_addr, ipaddr_buf[IPADDR_STRLEN];
	uint32_t sendrt_ip4addr, destrt_ip4addr, prev_ip4addr;
	struct sockaddr * recv_addr;
	char *vm_recv;
	char template[] = "/tmp/tmpfile_XXXXXX";
	char lhost_name[HOST_NAME_MAX];
	char canonical_ip_src[INET_ADDRSTRLEN];	
	
	fd_set rset;
	struct timeval tv;
	int		timeout_err; //timeout flag/error message
	uint8_t 	recv_flag = 0, //recieve flag
			me_flag = 0,   //me flag, for when node is start
			tour_flag = 1, //whether the application is still touring
			join_mcast_triflag = 0; //tristate flag; 0 - not part of group; 1 - indicator to join mcast group; 0xff -already part of group
	int ping_cnt=0, ping_stop = TO_EXIT;
	time_t rawtime;
	struct tm * timeinfo;		
	
	struct ping_node *ping_head=NULL, *ping_index, *ping_insert;

	get_lhostname(lhost_name, HOST_NAME_MAX);
	//get different random seeds
	srand(time(NULL)*getpid());
	if(argc > 2)
	{
#ifdef TEST //it is easier to test to run the same code on all machines
	        me_flag = (strcmp(lhost_name, argv[1])==0)?1:0; //am I the first node listed?
#else
        	me_flag =1; //greater than 2 args - can leave
#endif
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


	rtfd = bind_rt_socket(src_port);
	pingfd = create_ping_socket();      	
 
	sleep(START_DELAY); //make sure socket is open on all nodes before starting	
	
	if(me_flag) //compose first message with translate argv
	{
		//append itself to head	
		ip_addr = get_canonical_ip(lhost_name);
		for(j=0; j<MSGARG_LEN; j++){ ipaddr_buf[j]=0; }//empty bufferdd
		inet_pton(AF_INET, ip_addr, &sendrt_ip4addr);
		prev_ip4addr = sendrt_ip4addr = htonl(sendrt_ip4addr);
		memcpy(ipaddr_buf, &sendrt_ip4addr, IPADDR_LEN);
		memcpy(&rt_rmsg[msgarg_ind*MSGARG_LEN], ipaddr_buf, MSGARG_LEN);
		//create the rest of the list
		for(i=1; i<argc; i++)
		{
			msgarg_ind = i-repeats;
			ip_addr = get_canonical_ip(argv[i]);
			if(ip_addr == NULL || argv[i][0]!='v' || argv[i][1] != 'm')
			{
				fprintf(stderr, "Aborting: Invalid vm name: %s\n\tOther machines will hang\n", argv[i]);
				exit(0);
			}
			for(j=0; j<MSGARG_LEN; j++){ ipaddr_buf[j]=0; }//empty buffer
			inet_pton(AF_INET, ip_addr, &destrt_ip4addr);
			destrt_ip4addr = htonl(destrt_ip4addr);
			if(prev_ip4addr == destrt_ip4addr)
			{
				repeats++;
				fprintf(stderr, "Node %s: Tour has repeated nodes in sequence: %s\n\tIgnoring 2nd %s\n", lhost_name, argv[i], argv[i]);
				continue;
			}
			else{ prev_ip4addr = destrt_ip4addr; }
			memcpy(ipaddr_buf, &destrt_ip4addr, IPADDR_LEN);
			memcpy(&rt_rmsg[msgarg_ind*MSGARG_LEN], ipaddr_buf, MSGARG_LEN);
		}
		rt_len = (msgarg_ind+1)*MSGARG_LEN;
		if(join_mcast_triflag != 0xff){ join_mcast_triflag = 1;} //join mcast if not already a part of it
		me_flag=1; //still set
	}


	max_fd = (rtfd>max_fd)?(rtfd + 1):max_fd;
	max_fd = (pingfd>max_fd)?(pingfd + 1):max_fd;
	signal(SIGALRM, ping_alarm);
	while(tour_flag==1) //pinging
	{
		FD_ZERO(&rset);
		/* Send */
		if(me_flag==1)
		{
			memcpy(rt_smsg, &rt_rmsg[1*MSGARG_LEN], RT_MSG_LEN-MSGARG_LEN);
			
			memcpy(&sendrt_ip4addr, &rt_rmsg[0], IPADDR_LEN); //me (from recieved buffer)
			sendrt_ip4addr = ntohl(sendrt_ip4addr);
			
			memcpy(&destrt_ip4addr, &rt_smsg[0], IPADDR_LEN); //me (from send buffer)
			destrt_ip4addr = ntohl(destrt_ip4addr); 
			vm_recv = get_vm_name(destrt_ip4addr);
			if(vm_recv != NULL){ fprintf(stderr, "Node %s sending to %s:", lhost_name, vm_recv); }
			else{ fprintf(stderr, "VM recieve node not found:"); fprint_bytes(stderr, (char*)&destrt_ip4addr, IPADDR_LEN); }
			
			rt_len -= MSGARG_LEN;
			fprint_bytes(stderr, rt_smsg, rt_len);
			send_rt(rtfd, rt_smsg, rt_len, sendrt_ip4addr, destrt_ip4addr, get_eph_port());
			me_flag = 0; //reset flag
		}
		//join mcast group (one time only)
		if(join_mcast_triflag==1)
		{
                        send_mcastfd = create_mcast_socket(MCAST_ADDR, src_port, &mcast_sasend, &mcast_salen);

                        mcast_sarecv = Malloc(mcast_salen);
                        memcpy(mcast_sarecv, mcast_sasend, mcast_salen);
                        recv_mcastfd = Socket(mcast_sasend->sa_family, SOCK_DGRAM, 0);
			fprintf(stderr, "Node %s: Binding to Mcast\n", lhost_name);
                        Bind(recv_mcastfd, mcast_sarecv, mcast_salen);

                        Setsockopt(recv_mcastfd, SOL_SOCKET, SO_REUSEADDR, &so_d, sizeof(so_d));
 			Mcast_join(recv_mcastfd, mcast_sasend, mcast_salen, NULL, 0);
		        Mcast_set_loop(send_mcastfd, 0);

			max_fd = (recv_mcastfd>max_fd)?(recv_mcastfd + 1):max_fd;
			
			if(join_mcast_triflag != 0xff){ join_mcast_triflag = 0xff; }//can not be reset & rejoin mcast group
		}

		tv.tv_sec = (recv_flag==1)?TO_MCAST:TO_EXIT;
		tv.tv_usec = 0;

		if(ping_flag==1 && recv_flag==0)
		{
			fprintf(stderr, "Node %s \"Pinging\"\n", lhost_name); //ping code here
			for(ping_index = ping_head; ping_index !=NULL; ping_index = ping_index->next)
			{
				inet_ntop(AF_INET, &ping_index->ip4, (char*)&ipaddr_buf, IPADDR_STRLEN);
				ping(ipaddr_buf);
			}	
			ping_flag = 0;
			ping_cnt++;
			alarm(ALARM_RATE);
		}	
	
		FD_SET(rtfd, &rset);
		FD_SET(pingfd, &rset);
		if(join_mcast_triflag == 0xff){ FD_SET(recv_mcastfd, &rset); }
		
		timeout_err = select(max_fd, &rset, NULL, NULL, &tv); //is pselect better?

		if (timeout_err < 0) 
		{
			if(errno == EINTR && ping_cnt > ping_stop)
			{
		                snprintf(mcast_smsg, MCAST_MSG_LEN, TOUR_END_MSG, lhost_name);
	                        fprintf(stderr, "Node %s Sending: %s\n", lhost_name, mcast_smsg);
        	                send_mcast(send_mcastfd, mcast_smsg, mcast_sasend, mcast_salen);  /* parent -> sends */
				ping_flag = 0; //no more pinging
			}
			else if(errno == EINTR)
			{
				ping_flag=1;//make sure ping flag is set
			}
			else //unknown error
			{
				fprintf(stderr, "Abort: Node %s select error", lhost_name);
				exit(1);
			}
		}	
		else if(timeout_err == 0 && recv_flag==1) //no mcast recieved in a while end tour
		{
			
			tour_flag=0;
		}
		else if(timeout_err == 0) //timeout, but not a part of group - quit
                {

                        tour_flag=0;
                }

		else if(FD_ISSET(rtfd, &rset))
		{ 
			rt_len = RT_MSG_LEN;
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
	
			fprintf(stderr, "Time: %p\n\tNode %s Message Recieved From %s(%d bytes): ", asctime(timeinfo), lhost_name, vm_recv, rt_len);
			fprint_bytes(stderr, rt_rmsg, rt_len);
			if(rt_len < MSGARG_LEN*2) //we are the last argument - stop after a couple of pings
			{
				ping_stop = ping_cnt + NUM_PINGS; //stop pinging shortly
			}
			else
			{
				me_flag = 1; //send out new message
			}
			
			do{ ping_insert = malloc(sizeof(struct ping_node)); } while(ping_insert==NULL);
			ping_insert->ip4 = (uint32_t)((struct sockaddr_in*)recv_addr)->sin_addr.s_addr;
			ping_insert->next = NULL;
			if(ping_head==NULL){ ping_head = ping_insert; }
			else
			{
				for(ping_index=ping_head; ping_index!= NULL && ping_index->next != NULL; ping_index = ping_index->next)
				{
					if(ping_index->ip4 == ping_insert->ip4)
					{  
						ping_index=NULL; 
						free(ping_insert); 
						break; 
					}
				}
				if(ping_index!=NULL){ping_index->next = ping_insert; }
			}
			ping_insert = NULL;
			ping_flag=1;//we can start pinging now

			if(join_mcast_triflag != 0xff){ join_mcast_triflag = 1;} //join mcast if not already a part of it
		}
		else if(FD_ISSET(pingfd, &rset))
		{
			read_ping();
		}
		else if(FD_ISSET(recv_mcastfd, &rset))
		{
			 
                        recv_flag = 1;
                        recv_mcast(recv_mcastfd, mcast_rmsg, MCAST_MSG_LEN, mcast_salen);
                        fprintf(stderr, "Node %s Recieved: %s\n", lhost_name, mcast_rmsg);

                        if(strstr(mcast_rmsg, TOUR_END_MIDENTIFY) != NULL)
                        {
                                snprintf(mcast_smsg, MCAST_MSG_LEN, IDENTIFY_MSG, lhost_name);
                                fprintf(stderr, "Node %s Sending: %s\n", lhost_name,mcast_smsg);
                                send_mcast(send_mcastfd, mcast_smsg, mcast_sasend, mcast_salen);  /* parent -> sends */
                        }
			
			ping_flag = 0; //no more pinging
		}	
	}

	close(send_mcastfd);
	close(recv_mcastfd);
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


