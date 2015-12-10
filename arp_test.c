#define ADR_TEST
#ifdef ADR_TEST
#include "unp.h"
#include "hw_addrs.h"
#include "minix.h"
#include "params.h"
#include "domainsock.h"
#define TEST
#define ARPTEST_BUFSIZE 64

static void timeout_alarm(int signo){ signal(signo, timeout_alarm); return; }

int main(int argc, char **argv)
{
	int unix_rfd, maxfd;
	char template[64];
	char sbuf[ARPTEST_BUFSIZE], rbuf[ARPTEST_BUFSIZE], ipaddr[16];
	uint32_t ip4;	
	fd_set rset;	
	int i;

	srand(time(NULL)*getpid());
	sleep(ARP_START_DELAY);

	strncpy(template, CLIENT_FILE, 64);
	//mkstemp(template);
	unlink(template);
	unix_rfd = bind_unix_socket(template);
	connect_unix_socket(unix_rfd, template);
	sleep(1);
	strncpy(ipaddr, VM1_ADDR, 16);
	inet_pton(AF_INET,ipaddr, &ip4);
	ip4 = htonl(ip4);
	memcpy(sbuf, &ip4, 4);
	send_unix_reply(unix_rfd, sbuf, 4);
	maxfd = unix_rfd+1;

	FD_ZERO(&rset);
	FD_SET(unix_rfd, &rset);
	select(maxfd, &rset, NULL, NULL, NULL);
	fprintf(stderr, "Select Ended\n");
	recv_unix_req(unix_rfd, rbuf, ARPTEST_BUFSIZE);
	for(i=0; i<6; i++)
	{
		fprintf(stderr, "%x:", rbuf[i]);
	}
	fprintf(stderr, "\n");
	close(unix_rfd);
	unlink(template);
	return 0;
}
#else
int main(int argc, char **argv){ return 0;}
#endif
