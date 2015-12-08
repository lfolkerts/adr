#include "unp.h"
#include "mcast.h"
#include "minix.h"
#include <assert.h>
#include <stdint.h>
#include "hw_addrs.h"

#define ARRAY_SIZE(name) sizeof(name)/sizeof(name[0])

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
