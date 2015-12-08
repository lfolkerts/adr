#define ADR_TEST

#ifdef ADR_TEST
#include "unp.h"

#define TEST



int main(int argc, char **argv)
{
	int unix_rfd, pf_fd, max_fd;
	/* define as an array instead of a pointer to make the string writable */
	int flag = 0, count = 0; 
	char template[] = "/tmp/tmpfile_XXXXXX";
	char vm_name[HOST_NAME_MAX];
	char lhost_name[HOST_NAME_MAX];
	char *rhost_name, *canonical_ip;
	char canonical_ip_src[INET_ADDRSTRLEN];
	char res_buf[MAX_MSG_SZ];
	int src_port;
	char msg[] = "xx";
	uint16_t dest_port;
#ifdef TEST
	srand(time(NULL)*getpid());
	sleep(ARP_SLEEP_DELAY);
#endif
	/* initialize the sockets */

#ifndef TEST
	mkstemp(template);
	unlink(template);
	unix_rfd = bind_domain_socket(template);
#endif
	pf_fd = bind_pf_socket();

	hwahead = get_hw_addrs();
	global_hw_head = hwahead;
	
	for (hwa = hwahead; hwa != NULL; hwa = hwa->hwa_next) 
	{
		if(hwa->ip_alias == 0 && hwa->ip_loop == 0 && hwa->if_name[3] == '0')
		{
			memcpy(&cannon_ip, &((struct sockaddr_in *)hwa->ip_addr)->sin_addr, sizeof(struct in_addr));
	I		interface = hwa;
	   		break;
	 	}
     	}

	max_fd = (pf_fd > unix_rfd) ? pf_fd+1 : unix_rfd+1;

	signal(SIGALRM, timeout_alarm);
	alarm_flag=1;
	while (1) {

		FD_ZERO(&rset);
		FD_SET(pf_fd, &rset);
		FD_SET(unix_rfd, &rset);

		get_lhostname(lhost_name, sizeof(lhost_name));
	
		if(alarm_flag)
		{
			alarm(1);
		}	
		timeout_err = select(maxfd, &rset, NULL, NULL, NULL);
		if (timeout_err < 0)
                {
                        if(errno == EINTR) //alarm
                        {
                                close(unix_acceptfd);
				deleteTourInformation(tour_sockfd,1);
				unix_acceptfd = -1;     
                  	}
                        else //unknown error
                        {
                                fprintf(stderr, "Abort: Node %s select error", lhost_name);
                                exit(1);
                        }
                }
		else if(timeout_err == 0) //ligit timeout - no packet recieved in long time abort
                {
			close(unix_acceptfd);
			deleteTourInformation(tour_sockfd,1);
			unix_acceptfd = -1;
			replywait_flag=0; 
                }

		if(FD_ISSET(unix_rfd,&rset))
		{
			if(replywait_flag==1); //still awaiting reply- not sure how to handle this yet
			else
			{	
				if(unix_accept > 0)//close old socket and accept new one
				{
					close(unix_acceptfd);
				}
				unix_acceptfd = accept(unix_rfd, NULL, NULL);
			}
		}	
		if(FD_ISSET(unix_acceptfd, &rset))
		{
			recv_unix_req(unix_acceptfd, unix_reqbuf);
			memcpy(&ipreqaddr,tbuff,sizeof(struct sockaddr_in)); //copy over info
			rinfo  = lookup_cache_info(ipreqaddr.sin_addr.s_addr);

			if(rinfo)//found in cache
			{
				memcpy(unix_sbuf, &rinfo->dest_mac, sizeof(hwaddr));	
				send_unix_reply(unix_acceptfd, unix_sbuf);
				replywait_flag=0; 
			}
			else
			{
				create_cache_info(sa,interface,ipaddr.sin_addr.s_addr,NULL,&tour_sa,sd2);
				eth_shdr.h_proto = htons(GRP8_P_ARP);
				arp_shdr.src_ip = ((struct sockaddr_in *)interface->ip_addr)->sin_addr.s_addr;
				arp_shdr.dest_ip = ipreqaddr.sin_addr.s_addr;
				memcpy(arp_shdr.src_mac, eth_shdr.h_source,6);
				arp_shdr.op = OP_ARP_REQ;
   		           	send_arp(pf_fd, arp_shdr, eth_shdr,interface->if_index);
				replywait_flag = 1;
			}
		
		}
		if(FD_ISSET(pf_fd,&rset))
		{
			recv_arp(pf_fd, arp_rmsg);
			memcpy(&eth_shdr, arp_rmsg, sizeof(struct ethhdr));
			memcpy(&arp_rhdr, arp_rmsg + sizeof(struct ethhdr), sizeof(arprr));
			src = ether_ntoa((struct ether_addr *)&(eth_shdr.h_source));
			dst = ether_ntoa((struct ether_addr *)&(eth_shdr.h_dest));
			
			rinfo = lookup_cache_info(arp_rhdr.dest_ip);
			if(arp_rhdr.dest_ip != cannon_ip.s_addr)
			{
				fprintf(stderr, "Wrong node\n");
			}
			else if(arp_rhdr.op == ARP_REQ)
			{
				
				create_cache_info(rcv_sa,interface,arp_rhdr.src_ip,arp_rhdr.src_mac,NULL,-1);
				memcpy(&eth_rhdr,&eth_shdr,sizeof(struct ethhdr));	
				memcpy(eth_shdr.h_source,rinfo->dest_mac.sll_addr,6);
				memcpy(eth_shdr.h_dest,eth_rhdr.h_source,sizeof(struct ether_addr));
				memcpy(&arp_shdr,&arp_rhdr,sizeof(arprr));
				
				arp_shdr.src_ip= arp_rhdr.dest_ip;
				arp_shdr.dest_ip= arp_rhdr.src_ip;
				memcpy(arp_shdr.src_mac,rinfo->dest_mac.sll_addr,6);
				memcpy(arp_shdr.dest_mac,eth_shdr.h_dest,sizeof(arp_rhdr.src_mac));
				arp_rhdr.op =  OP_ARP_REP;

				send_arp(pf_fd, arp_hdr, eth_hdr,interface->if_index);
			}
			else if(arp_rhdr.op == ARP_REP)
			{
				dst_mac = ether_ntoa((struct ether_addr *)&(eth_shdr.h_source));
				create_cahce_info(rcv_sa,interface,arp_rhdr.src_ip,arp_rhdr.src_mac,NULL,-1);

				if(replywaiting_flag==1)
				{
					send_unix_reply();
				}
				else //timed out - make sure destroyed
				{
					destroyRouteInformation(arp_rhdr.src_ip);
				}
			}

			replywait_flag=0; //got our reply
		}
	


		

	}//while 1
	unlink(template);
	return 0;
}

#endif
