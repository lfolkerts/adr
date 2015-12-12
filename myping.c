#include "myping.h"
#include "in_cksum.h"

struct proto	proto_v4 = { proc_v4, send_v4, NULL, NULL, 0, IPPROTO_ICMP };

int	datalen = 56;		/* data that goes with ICMP echo request */

/* globals */
char	 sendbuf[BUFSIZE];

int datalen;			/* # bytes of data following ICMP header */
char*host;
int nsent;				/* add 1 for each sendto() */
pid_t pid;				/* our PID */
int sockfd;

int create_ping_socket()
{
	
	int size;
	
	pr = &proto_v4;

	sockfd = Socket(AF_INET, SOCK_RAW, pr->icmpproto);
	setuid(getuid());		/* don't need special permissions any more */
	
	size = 60 * 1024;		/* OK if setsockopt fails */
	setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
	return sockfd;
}

int ping(char* myhost)
{
	int c;
	struct addrinfo	*ai;
	char *h;

	if(myhost == NULL) return -1;
	do{ host = malloc(strlen(myhost)+1); }while(host == NULL);
	strcpy(host, myhost);

	pid = getpid() & 0xffff;	/* ICMP ID field is 16 bits */

	ai = Host_serv(host, NULL, 0, 0);

	h = Sock_ntop_host(ai->ai_addr, ai->ai_addrlen);
	printf("PING %s (%s): %d data bytes\n",
			ai->ai_canonname ? ai->ai_canonname : h,
			h, datalen);
	
	pr->sarecv = Calloc(1, ai->ai_addrlen);
	pr->salen = ai->ai_addrlen;
	pr->sasend = ai->ai_addr;

	send_v4();
	return 0;	
}

void read_ping(void)
{
	int				size;
	char			recvbuf[BUFSIZE];
	char			controlbuf[BUFSIZE];
	struct msghdr	msg;
	struct iovec	iov;
	ssize_t			n;
	struct timeval	tval;

	iov.iov_base = recvbuf;
	iov.iov_len = sizeof(recvbuf);
	msg.msg_name = pr->sarecv;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	msg.msg_control = controlbuf;
	msg.msg_namelen = pr->salen;
	msg.msg_controllen = sizeof(controlbuf);
	n = recvmsg(sockfd, &msg, 0);
	if (n < 0) {
		if (errno == EINTR)
			return read_ping();
		else
			err_sys("recvmsg error");
	}

	Gettimeofday(&tval, NULL);
	(*pr->fproc)(recvbuf, n, &msg, &tval);
}

void send_v4(void)
{
	int len;
	struct icmp *icmp;

	icmp = (struct icmp *) sendbuf;
	icmp->icmp_type = ICMP_ECHO;
	icmp->icmp_code = 0;
	icmp->icmp_id = pid;
	icmp->icmp_seq = nsent++;
	memset(icmp->icmp_data, 0xa5, datalen);	/* fill with pattern */
	Gettimeofday((struct timeval *) icmp->icmp_data, NULL);

	len = 8 + datalen;		/* checksum ICMP header and data */
	icmp->icmp_cksum = 0;
	icmp->icmp_cksum = in_cksum((u_short *) icmp, len);

	Sendto(sockfd, sendbuf, len, 0, pr->sasend, pr->salen);
}

void proc_v4(char *ptr, ssize_t len, struct msghdr *msg, struct timeval *tvrecv)
{
	int hlen1, icmplen;
	double	rtt;
	struct ip *ip;
	struct icmp *icmp;
	struct timeval *tvsend;

	ip = (struct ip *) ptr;		/* start of IP header */
	hlen1 = ip->ip_hl << 2;		/* length of IP header */
	if (ip->ip_p != IPPROTO_ICMP)
		return;				/* not ICMP */

	icmp = (struct icmp *) (ptr + hlen1);	/* start of ICMP header */
	if ( (icmplen = len - hlen1) < 8)
		return;				/* malformed packet */

	if (icmp->icmp_type == ICMP_ECHOREPLY) {
		if (icmp->icmp_id != pid)
			return;			/* not a response to our ECHO_REQUEST */
		if (icmplen < 16)
			return;			/* not enough data to use */

		tvsend = (struct timeval *) icmp->icmp_data;
		tv_sub(tvrecv, tvsend);
		rtt = tvrecv->tv_sec * 1000.0 + tvrecv->tv_usec / 1000.0;

		printf("%d bytes from %s: seq=%u, ttl=%d, rtt=%.3f ms\n",
				icmplen, Sock_ntop_host(pr->sarecv, pr->salen),
				icmp->icmp_seq, ip->ip_ttl, rtt);

	}
}
