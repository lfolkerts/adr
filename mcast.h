#ifndef MCAST_H__
#define MCAST_H__

#include <stdint.h>
#define SERVER_PORT 31337
#define SERVER_SUNPATH "/tmp/odrserver_3254"

int create_mcast_socket(char* mcast_ip, int port, struct sockaddr** saudp, socklen_t* saudp_len);

void send_mcast(int sendfd, char* buf, SA *sadest, socklen_t salen);
struct sockaddr* recv_mcast(int recvfd, char* buf, int maxsize, socklen_t salen);

#endif

