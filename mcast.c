#include "unp.h"
#include "mcast.h"
#include <assert.h>
#include <stdint.h>
#include "hw_addrs.h"

#define ARRAY_SIZE(name) sizeof(name)/sizeof(name[0])


/* storing index might seem redundant, indeed we can get away with an array if char *, this is just
   for maintable code, where vm's indexes are not necessarily starting from 1 */
struct vm_addr_struct vm[] = {
	{.idx = VM1_IDX,  .addr = VM1_ADDR},
	{.idx = VM2_IDX,  .addr = VM2_ADDR},
	{.idx = VM3_IDX,  .addr = VM3_ADDR},
	{.idx = VM4_IDX,  .addr = VM4_ADDR},
	{.idx = VM5_IDX,  .addr = VM5_ADDR},
	{.idx = VM6_IDX,  .addr = VM6_ADDR},
	{.idx = VM7_IDX,  .addr = VM7_ADDR},
	{.idx = VM8_IDX,  .addr = VM8_ADDR},
	{.idx = VM9_IDX,  .addr = VM9_ADDR},
	{.idx = VM10_IDX, .addr = VM10_ADDR},
};



int bind_domain_socket(char *path)
{
	struct sockaddr_un addr;
	int sockfd = Socket(AF_LOCAL, SOCK_DGRAM, 0 );

	memset(&addr, 0x0, sizeof(addr));
	unlink(path);
	addr.sun_family = AF_LOCAL;
	strcpy(addr.sun_path, path);
	Bind(sockfd, (void *) &addr, sizeof(addr));
	return sockfd;
}
/*Modified parts from lib/udp_client.c*/
int bind_mcast_socket(char* mcast_ip, int port, struct sockaddr** saudp, socklen_t* saudp_len)
{
	int sockfd, err;
	struct addrinfo hints, *res;
	char sport[10];

	bzero(&hints, sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	
	snprintf(sport, 10, "%d", port);
	
	err = getaddrinfo(mcast_ip, sport, &hints, &res);
	if(err<0)
	{
		fprintf(stderr, "getaddrinfo err");
	}

	do 
	{
		sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sockfd >= 0)
			break;          /* success */
	} while ( (res = res->ai_next) != NULL);

	if (res == NULL)        /* errno set from final socket() */
	{
		err_sys("udp_client error;");
	}
	
	*saudp = Malloc(res->ai_addrlen);
	memcpy(*saudp, res->ai_addr, res->ai_addrlen);
	*saudp_len = res->ai_addrlen;

	//Bind(sockfd, (void *) *saudp, sizeof(struct sockaddr));
	//fprintf(stdout, "Bound %d\n", sockfd);
	return sockfd;
}


void connect_domain_socket(int sockfd, char *path)
{
	struct sockaddr_un addr;

	memset(&addr, 0x0, sizeof(addr));
	addr.sun_family = AF_LOCAL;
	strcpy(addr.sun_path, path);
	Connect(sockfd, (void *) &addr, sizeof(addr));
}

void connect_mcast_socket(int sockfd, char *path)
{
	struct sockaddr_un addr;

	memset(&addr, 0x0, sizeof(addr));
	addr.sun_family = AF_INET;
	strcpy(addr.sun_path, path);
	Connect(sockfd, (void *) &addr, sizeof(addr));
}


static char *get_canonical_ip(char *name)
{
	/* skip the "vm" part */
	char *idx = &name[2];
	int index, i;

	index = atoi(idx);
	assert(index > 0 && index <= 10);
	/* array is zero based */
	index -= 1;
	for (i = 0; i < ARRAY_SIZE(vm); i++) {
		if (vm[i].idx == index)
			return vm[i].addr;
	}
	return NULL;
}

void get_lhostname(char *buf, size_t len)
{
	int ret;

	/* get hostname doesn't guarantee null termination */
	buf[len -1] = 0x0;
	ret = gethostname(buf, len - 1);
	if (ret == -1) {
		printf("error in gethostname\n");
		exit(-1);
	}
	return ;
}

uint16_t get_eph_port()
{
	/* we rely on randomness to make sure we get a unique eph port */
	return (uint16_t ) rand();
}

int _select(int nfds, fd_set *rset, fd_set *wset, fd_set *eset, struct timeval *tv)
{
	int ret;
again:
	if ( (ret = select(nfds, rset, wset, eset, tv)) < 0) {
		if (errno == EINTR)
			goto again;
		else
			err_sys("Select error\n");
	}
	return ret;
}

static char *get_sunpath(int sockfd)
{
	struct sockaddr_un *s = Calloc(1, sizeof(*s));
	socklen_t len = sizeof(*s);
	Getsockname(sockfd, (void *)s, &len);
	return s->sun_path;
}

char *get_name(char *str)
{
	struct in_addr addr;
	struct hostent *h;

	memset(&addr, 0x0, sizeof(addr));
	inet_pton(AF_INET, str, &addr);
	h = gethostbyaddr((void *) &addr, sizeof(addr), AF_INET);
	return h->h_name;
}

void send_mcast(int sendfd, char* buf, SA *sadest, socklen_t salen)
{
	Sendto(sendfd, buf, strlen(buf), 0, sadest, salen);
}

struct sockaddr* recv_mcast(int recvfd, char* buf, int maxsize, socklen_t salen)
{
	int n;
	socklen_t len;
	struct sockaddr *safrom;

	safrom = Malloc(salen);
	len = salen;
	n = Recvfrom(recvfd, buf, maxsize-1, 0, safrom, &len);
	buf[n] = 0;	/* null terminate */

	return safrom;
}
