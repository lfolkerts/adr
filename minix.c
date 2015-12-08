#include "unp.h"
#include "minix.h"
#include "params.h"
#include <assert.h>
#include <stdint.h>
#include "hw_addrs.h"

#define ARRAY_SIZE(name) sizeof(name)/sizeof(name[0])
#define EPH_PORT 9049
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

char *get_canonical_ip(char *name)
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
	return (uint16_t ) EPH_PORT;
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
char *get_vm_name(uint32_t ip4)
{
	uint32_t i, ip4_cmp;
	for(i=0; i<NUM_VMS; i++)
	{
		inet_pton(AF_INET, vm[i].addr, &ip4_cmp);
		if(ip4==ip4_cmp){ break; }
	}
	if(i<NUM_VMS){ return vm[i].addr;}
	else{ return NULL; }
}
char* get_name(uint32_t ip4)
{
	struct sockaddr_in addr;
	struct hostent *h;

	addr.sin_addr.s_addr  = ip4;
	h = gethostbyaddr((struct sockaddr *) &addr, sizeof(addr), AF_INET);
	if(h!=NULL)
	{
		fprintf(stderr, "%s\n", h->h_name);
	}
	else
	{
		fprintf(stderr, "VM not found\n");
	}
	return h->h_name;
}

