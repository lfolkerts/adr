#ifndef RT_H__
#define RT_H__

#include <stdint.h>
#define SERVER_PORT 31337
#define SERVER_SUNPATH "/tmp/odrserver_3254"

int bind_rt_socket(int port);
void send_rt(int sendfd, char* buf, int msg_len, uint32_t ip4src, uint32_t ip4dest, uint16_t dest_port);
struct sockaddr* recv_rt(int recvfd, char* buf, int* maxsize, socklen_t salen);

#endif

