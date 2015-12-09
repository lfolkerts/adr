#define ADR_TEST
#ifdef ADR_TEST
#include "unp.h"
#include <linux/if_packet.h>
#include <linux/if_ether.h> 
#include <netinet/ether.h>
#include "arp.h"
#include "hw_addrs.h"
#include "arpcache.h"
#include "params.h"
#define TEST
#define IP_LEN 4
#define IP_STRLEN 16
#define UNIX_BUFLEN 64
#define ID_LEN 2
#define ETH_LEN 6

int main(int argc, char **argv)
{
	int unix_rfd, unix_acceptfd, pf_fd, max_fd;
	char template[] = "/tmp/tmpfile_XXXXXX";
	char vm_name[HOST_NAME_MAX];
	char lhost_name[HOST_NAME_MAX];
	int src_port, dest_port;
	char arp_msg[ID_LEN];	

	struct hwa_info* hwahead, *hwa, *ifi;
	char ip4 [IP_LEN];
	char unix_reqbuf[UNIX_BUFLEN], unix_sbuf[UNIX_BUFLEN];
	struct sockaddr_in ipaddrreq;
	SA* unix_acceptsa;
	struct arphdr *arp_shdr, *arp_rhdr;
	struct ether_arp *eth_shdr,*eth_rhdr, *srceth, *dsteth;
	char arp_rmsg[UNIX_BUFLEN];
	fd_set rset;	
	uint8_t replywait_flag = 0;
	int timeout_err;
	char ethsrc[ETH_LEN], ethdest[ETH_LEN];
#ifdef TEST
	srand(time(NULL)*getpid());
	sleep(ARP_START_DELAY);
#endif
	/* initialize the sockets */

#ifndef TEST
	mkstemp(template);
	unlink(template);
	unix_rfd = bind_domain_socket(template);
#endif
	pf_fd = bind_pf_socket();

	hwahead = get_hw_addrs();
	
	for (hwa = hwahead; hwa != NULL; hwa = hwa->hwa_next) 
	{
		if(hwa->ip_alias == 0 && hwa->ip_l.id == 0 && hwa->if_name[3] == '0')
		{
			memcpy(&ip4, &((struct sockaddr_in *)hwa->ip_addr)->sin_addr.s_addr, IP_LEN);
	I		ifi = hwa;
	   		break;
	 	}
     	}

	max_fd = (pf_fd > unix_rfd) ? pf_fd+1 : unix_rfd+1;

	signal(SIGALRM, &timeout_alarm);
	get_lhostname(lhost_name, sizeof(lhost_name));
	unix_acceptfd = -1;
	while (1) {

		FD_ZERO(&rset);
		FD_SET(pf_fd, &rset);
		FD_SET(unix_rfd, &rset);
		if(unix_acceptfd > 0)
		{
			FD_SET(unix_acceptfd, &rset);	
		}
	
		if(replywait_flag==0)
		{
			alarm(ARP_ALARM);
		}	
		timeout_err = select(max_fd, &rset, NULL, NULL, NULL);
		if (timeout_err < 0)
                {
                        if(errno == EINTR) //alarm
                        {
                                close(unix_acceptfd);
				delete_cache_entry(unix_acceptfd);
				unix_acceptfd = -1;     
				replywait_flag=0; 
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
			remove_arp_entry(unix_acceptfd,1);
			unix_acceptfd = -1;
			replywait_flag=0; 
                }

		if(FD_ISSET(unix_rfd,&rset))
		{
			if(replywait_flag==1) //still awaiting reply- not sure how to handle this yet dont want to fork
			{
				fprintf(stderr, "ARP request ignored - already processing a requesti\n");
			}
			else
			{	
				if(unix_acceptfd > 0)//close old socket and accept new one
				{
					close(unix_acceptfd);
				}
				unix_acceptfd = accept(unix_rfd, NULL, NULL);
				replywait_flag = 1;
			}
		}	
		if(FD_ISSET(unix_acceptfd, &rset))
		{
			recv_unix_req(unix_acceptfd, unix_reqbuf);
			memcpy(&ipreqaddr,unix_reqbuf,sizeof(struct sockaddr_in)); //.idy over info
			arpi  = lookup_cache_info(ipreqaddr.sin_addr.s_addr);

			if(arpi != NULL)//found in cache
			{
				memcpy(unix_sbuf, &arpi->dest_mac, sizeof(hwaddr));	
				send_unix_reply(unix_acceptfd, unix_sbuf);
				replywait_flag=0; 
			}
			else
			{
				create_cache_info(ifi,ipaddr.sin_addr.s_addr,NULL,&unix_accept_sa);
				eth_shdr.h_proto = htons(ARPPROTO);
				arp_shdr.src_ip = ((struct sockaddr_in *)interface->ip_addr)->sin_addr.s_addr;
				arp_shdr.dest_ip = ipreqaddr.sin_addr.s_addr;
				memcpy(arp_shdr.src_mac, eth_shdr.h_source,6);
				arp_shdr.type = ARP_REQ_TYPE;
   		           	send_arp(pf_fd, arp_shdr, eth_shdr,interface->if_index);
				unix_acceptfd = -1;
				replywait_flag = 0; //so long we have the fd in cahce we can take another request
			}
		
		}
		if(FD_ISSET(pf_fd,&rset))
		{
			recv_addr = (struct sockaddr_ll*)recv_arp(pf_fd, arp_rmsg);
			memcpy(arp_rhdr, arp_rmsg, sizeof(struct arphdr));
			
			arpi = lookup_cache_info(arp_rhdr->e.arp_tpa);

			if(arp_rhdr->e.arp_op == ARP_REQ_TYPE ) //we dont have the entry in our cache - disregard
			{
				do{ arpi = malloc(sizeof(struct arp_node)); }while(apri==NULL);
				arpi->ip4 = ntohl(arp_rhdr->e.arp_spa);
				memcpy(arpi->hwa, arp_rhdr->e.arp_sha, ETH_ALEN)
				arpi->fd = -1; //not our fd
				arpi->sll_hatype = recv_addr->sll_hatype;
				arpi->sll_ifindex =  recv_addr->sll_ifindex;
				create_cache_entry(arpi);
				
				arpi = lookup_cache_info(arp_rhdr->e.arp_tpa);
			
				if(arpi->ip4 == my_ip)
				{		
					
					memcpy(arp_shdr->eth.h_source, arpi->hwa.sll_addr, ETH_ALEN);
					memcpy(arp_shdr->eth.h_dest, arp_rhdr->eth.h_source,sizeof(struct ether_addr));
					memcpy(&arp_shdr,&arp_rhdr,sizeof(arprr));
				
					arp_shdr.src_ip= arp_rhdr.dest_ip;
					arp_shdr.dest_ip= arp_rhdr.src_ip;
					arp_rhdr->type =  ARP_REPLY_TYPE;
					arp_rhdr->id = ARP_ID;
			
					send_arp(pf_fd, arp_shdr, ifi->if_index, NULL, 0);
				}
			}
			else if(arp_rhdr->type == ARP_REPLY_TYPE)
			{
				dsteth = ether_ntoa((struct ether_addr *)&(eth_shdr.h_source));
				arpi = create_cache_entry(rcv_sa,ifi,arp_rhdr.src_ip,arp_rhdr.src_mac,NULL,-1);

				if(replywait_flag==1)
				{
					send_unix_reply(unix_acceptfd, arpi, arpi->size);
				}
				else //timed out - make sure destroyed
				{
					delete_cache_entry(arp_rhdr.src_ip);
				}
			}

			replywait_flag=0; //got our reply
		}
	


		

	}//while 1
	unlink(template);
	return 0;
}

#endif
