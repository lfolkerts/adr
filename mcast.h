#include "unp.h"

void send_mcast(int sendfd, char* buf, SA *sadest, socklen_t salen);
struct sockaddr* recv_mcast(int recvfd, socklen_t salen, char* buf, int maxsize);
