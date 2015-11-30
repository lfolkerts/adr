#include "unp.h"
#include "mcast.h"

void send_mcast(int sendfd, char* buf, SA *sadest, socklen_t salen)
{
	Sendto(sendfd, buf, strlen(buf), 0, sadest, salen);
}

struct sockaddr* recv_mcast(int recvfd, socklen_t salen, char* buf, int maxsize)
{
	int n;
	socklen_t len;
	struct sockaddr *safrom;

	safrom = Malloc(salen);
	len = salen;
	n = Recvfrom(recvfd, buf, size-1, 0, safrom, &len);
	buf[n] = 0;	/* null terminate */

	return safrom;
}
