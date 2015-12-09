#include "unp.h"
#include"domainsock.h"

static char *get_sunpath(int sockfd);

int bind_unix_socket(char *path)
{
        struct sockaddr_un addr;
        int sockfd = Socket(AF_LOCAL, SOCK_STREAM, 0 );

        memset(&addr, 0x0, sizeof(addr));
        unlink(path);
        addr.sun_family = AF_LOCAL;
        strcpy(addr.sun_path, path);
        Bind(sockfd, (void *) &addr, sizeof(addr));
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

int send_unix_reply(int sockfd, char *msg, int msg_len)
{
	struct sockaddr_un addr;

        memset(&addr, 0x0, sizeof(addr));
    
        /* sendto to the domain socket */
        addr.sun_family = AF_LOCAL;
	addr.sun_path = get_sunpath(sockfd);
        Sendto(sockfd, msg, msg_len, 0, (void *) &addr, sizeof(addr));
        return;

}


int recv_unix_req(int sockfd, char* msg, int msg_len)
{
        struct sockaddr_un addr;
	addr.sun_family = AF_LOCAL;
        addr.sun_path = get_sunpath(sockfd);
        socklen_t len = sizeof(addr);
        Recvfrom(sockfd, msg, msglen, 0, (void *) &addr, &len);
	return 0;

}

static char *get_sunpath(int sockfd)
{
        struct sockaddr_un *s = Calloc(1, sizeof(*s));
        socklen_t len = sizeof(*s);
        Getsockname(sockfd, (void *)s, &len);
        return s->sun_path;
}

