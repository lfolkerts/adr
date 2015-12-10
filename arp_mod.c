#define ADR_TEST
#ifdef ADR_TEST
#include "unp.h"
#include <string.h>
#include <linux/if_ether.h> 
#include <netinet/ether.h>
#include <linux/if_packet.h>
#include "arp.h"
#include "hw_addrs.h"
#include "minix.h"
#include "arpcache.h"
#include "params.h"
#include "domainsock.h"
#define TEST
#define IP_LEN 4
#define IP_STRLEN 16
#define UNIX_BUFLEN 64
#define ID_LEN 2
#define ETH_LEN ETH_ALEN

static void timeout_alarm(int signo){ signal(signo, timeout_alarm); return; }

int main(int argc, char **argv)
{
	int unix_rfd, unix_acceptfd, pf_fd, max_fd;
	char template[64];
	char vm_name[HOST_NAME_MAX];
	char lhost_name[HOST_NAME_MAX];
	int src_port, dest_port;

	struct hwa_info* hwahead, *hwa, *eth_ifi;
	uint32_t my_ip;
	char unix_reqbuf[UNIX_BUFLEN], unix_sbuf[UNIX_BUFLEN];
	struct sockaddr_in ipaddrreq;
	SA* unix_acceptsa;
	struct arp_node* arpi;
	struct myarphdr *arp_shdr=NULL, *arp_rhdr=NULL;
	char arp_rmsg[UNIX_BUFLEN];
	fd_set rset;	
	uint8_t replywait_flag = 0;
	int timeout_err;
	char my_ether[ETH_ALEN];
	struct sockaddr_in ipreqaddr;
	struct sockaddr_ll* recv_addrll;

#ifdef TEST
	srand(time(NULL)*getpid());
	sleep(ARP_START_DELAY);
#endif
	/* initialize the sockets */
	strncpy(template,SERVER_FILE, 64);
//	mkstemp(template);
	unlink(template);
	unix_rfd = bind_unix_socket(SERVER_FILE);

	arpInit();

	pf_fd = bind_pf_socket();

	hwahead = get_hw_addrs();

	for (hwa = hwahead; hwa != NULL; hwa = hwa->hwa_next) 
	{
		if(hwa->ip_alias == 0  && strstr(hwa->if_name, "eth0"))
		{
			memcpy(&my_ip, &hwa->ip_addr->sa_data, IP_LEN);
			memcpy(my_ether, hwa->if_haddr, ETH_ALEN);
			
			do{ arpi = malloc(sizeof(struct arp_node)); }while(arpi==NULL);
                        arpi->ip4 = my_ip;
                        memcpy(arpi->hwa, my_ether, ETH_ALEN);
                        arpi->fd = -1; //not our fd
                        arpi->sll_hatype = ARPHRD_ETHER;
                        arpi->sll_ifindex =  hwa->if_index;
                        create_cache_entry(arpi);
			arpi=NULL;

			eth_ifi = hwa;
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
			alarm(ARP_ALARM_RATE);
		}	
		timeout_err = select(max_fd, &rset, NULL, NULL, NULL);
		if (timeout_err < 0)
		{
			if(errno == EINTR) //alarm
			{
				close(unix_acceptfd);
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
			unix_acceptfd = -1;
			replywait_flag=0; 
		}

		if(FD_ISSET(unix_rfd,&rset))
		{
			/*
			if(replywait_flag==1) //still awaiting reply- not sure how to handle this yet dont want to fork
			{
				fprintf(stderr, "ARP request ignored - already processing a requesti\n");
			}
			else*/
			{	
				if(unix_acceptfd > 0)//close old socket and accept new one
				{
					close(unix_acceptfd);
				}
				unix_acceptfd = accept(unix_rfd, NULL, NULL);
				replywait_flag = 1;
			}
		}	
		if(unix_acceptfd>0 && FD_ISSET(unix_acceptfd, &rset))
		{
			recv_unix_req(unix_acceptfd, unix_reqbuf, UNIX_BUFLEN);
			memcpy(&ipreqaddr.sin_addr.s_addr,unix_reqbuf,sizeof(struct sockaddr_in)); //.idy over info
			arpi  = lookup_cache_entry(ntohl(ipreqaddr.sin_addr.s_addr));

			if(arpi != NULL)//found in cache
			{
				memcpy(unix_sbuf, &arpi->hwa, ETH_ALEN);	
				send_unix_reply(unix_acceptfd, unix_sbuf, ETH_ALEN);
				replywait_flag=0; 
			}
			else
			{
				do{ arpi = malloc(sizeof(struct arp_node)); }while(arpi==NULL);
				arpi->ip4 = ntohl(ipreqaddr.sin_addr.s_addr);
				//arpi->hwa;
				arpi->fd = unix_acceptfd; //not our fd
				//arpi->sll_hatype; 
				//arpi->sll_ifindex;

				create_cache_entry(arpi);

				memcpy(arp_shdr->eth.ether_shost, my_ether, ETH_ALEN);
				memset(arp_shdr->eth.ether_dhost, 0xFF, ETH_ALEN);
				arp_shdr->eth.ether_type = htons(ARP_TYPE);

				arp_shdr->e.arp_hrd = ETHERTYPE_IP;
				arp_shdr->e.arp_pro = 4; //IPv4
				arp_shdr->e.arp_hln = ETH_ALEN;
				arp_shdr->e.arp_pln = IP_LEN;
				arp_shdr->e.arp_op = htons(ARP_REQ_TYPE);
				memcpy(arp_shdr->e.arp_sha, my_ether, ETH_ALEN);
				memcpy(arp_shdr->e.arp_spa, &arpi->ip4, IP_LEN);
				memset(arp_shdr->e.arp_tha, 0, ETH_ALEN);//unknown
				memcpy(arp_shdr->e.arp_tpa, &ipreqaddr.sin_addr.s_addr, IP_LEN);
				arp_shdr->id = htons(ARP_ID);

				send_arp(pf_fd, arp_shdr, eth_ifi->if_index, NULL, 0);

				unix_acceptfd = -1;
				replywait_flag = 0; //so long we have the fd in cahce we can take another request
			}

		}
		if(FD_ISSET(pf_fd,&rset))
		{
			recv_addrll = (struct sockaddr_ll*)recv_arp(pf_fd, arp_rmsg, UNIX_BUFLEN );
			memcpy(arp_rhdr, arp_rmsg, sizeof(struct myarphdr));
			memcpy(&ipreqaddr.sin_addr.s_addr, arp_rhdr->e.arp_spa, IP_LEN);
			delete_empty_cache_entry(ntohl(ipreqaddr.sin_addr.s_addr));

			do{ arpi = malloc(sizeof(struct arp_node)); }while(arpi==NULL);
			arpi->ip4 = ntohl(ipreqaddr.sin_addr.s_addr);
			memcpy(arpi->hwa, arp_rhdr->e.arp_sha, ETH_ALEN);
			arpi->fd = -1; //not our fd
			arpi->sll_hatype = recv_addrll->sll_hatype;
			arpi->sll_ifindex =  recv_addrll->sll_ifindex;
			create_cache_entry(arpi);

			if(arp_rhdr->e.arp_op == ARP_REQ_TYPE ) //we dont have the entry in our cache - disregard
			{
				memcpy(&ipreqaddr.sin_addr.s_addr, arp_rhdr->e.arp_tpa, IP_LEN);
				arpi = lookup_cache_entry(ntohl(ipreqaddr.sin_addr.s_addr));
				if(arpi != NULL && arpi->ip4 == my_ip)
				{		

					memcpy(arp_shdr->eth.ether_shost, arpi->hwa, ETH_ALEN);
					memcpy(arp_shdr->eth.ether_dhost, arp_rhdr->eth.ether_shost, ETH_ALEN);
					arp_shdr->eth.ether_type = htons(ARP_TYPE);

					arp_shdr->e.arp_hrd = ETHERTYPE_IP;
					arp_shdr->e.arp_pro = 4; //IPv4
					arp_shdr->e.arp_hln = ETH_ALEN;
					arp_shdr->e.arp_pln = IP_LEN;
					arp_shdr->e.arp_op = htons(ARP_REPLY_TYPE);
					memcpy(arp_shdr->e.arp_sha,  my_ether, ETH_ALEN);
					ipreqaddr.sin_addr.s_addr = htonl(my_ip);
					memcpy(arp_shdr->e.arp_spa, &ipreqaddr.sin_addr.s_addr, IP_LEN);
					memcpy(arp_shdr->e.arp_tha, arp_rhdr->e.arp_sha, ETH_ALEN);
					memcpy(arp_shdr->e.arp_tpa, arp_rhdr->e.arp_spa, IP_LEN); 
					arp_shdr->id = htons(ARP_ID);

					send_arp(pf_fd, arp_shdr, eth_ifi->if_index, NULL, 0);
				}
			}
			else if(arp_rhdr->e.arp_op == ARP_REPLY_TYPE)
			{
				send_unix_reply(unix_acceptfd, arpi->hwa, ETH_ALEN);
			}
			replywait_flag=0; //got our reply
		}





	}//while 1
	unlink(template);
	return 0;
}

#endif
