#include	"unp.h"
#include	<netinet/in_systm.h>
#include	<netinet/ip.h>
#include	<netinet/ip_icmp.h>

#define	BUFSIZE		1500

			/* function prototypes */
int ping(char* host);
int create_ping_socket();
void proc_v4(char *, ssize_t, struct msghdr *, struct timeval *);
void send_v4(void);
void read_ping(void);

struct proto {
  void	 (*fproc)(char *, ssize_t, struct msghdr *, struct timeval *);
  void	 (*fsend)(void);
  struct sockaddr  *sasend;	/* sockaddr{} for send, from getaddrinfo */
  struct sockaddr  *sarecv;	/* sockaddr{} for receiving */
  socklen_t	    salen;		/* length of sockaddr{}s */
  int	   	    icmpproto;	/* IPPROTO_xxx value for ICMP */
} *pr;

struct ping_node
{
	uint32_t ip4;
	struct ping_node* next;
};
