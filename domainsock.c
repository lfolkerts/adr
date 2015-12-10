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
	char* sun;
        memset(&addr, 0x0, sizeof(addr));
    
        /* sendto to the domain socket */
 	sun = get_sunpath(sockfd);
	strncpy(addr.sun_path, sun,  108);
        addr.sun_family = AF_LOCAL;
        Sendto(sockfd, msg, msg_len, 0, (void *) &addr, sizeof(addr));
        return 0;

}


int recv_unix_req(int sockfd, char* msg, int msg_len)
{
        struct sockaddr_un addr;
	char* sun;
	socklen_t len;

	addr.sun_family = AF_LOCAL;
	sun = get_sunpath(sockfd);
	strncpy(addr.sun_path, sun,  108);
        

	len = sizeof(addr);
        Recvfrom(sockfd, msg, msg_len, 0, (void *) &addr, &len);
	return 0;

}

static char *get_sunpath(int sockfd)
{
        struct sockaddr_un *s = Calloc(1, sizeof(*s));
        socklen_t len = sizeof(*s);
        Getsockname(sockfd, (void *)s, &len);
        return s->sun_path;
}

