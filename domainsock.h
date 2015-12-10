#ifndef __DOMAINSOCK_H__
#define __DOMAINSOCK_H__

#include "unp.h"
#define CLIENT_FILE "tmp/template9049"
#define SERVER_FILE "tmp/template9050"

int bind_unix_socket(char *path);
void connect_unix_socket(int sockfd, char *path);
int send_unix_reply(int sockfd, char *msg, int msg_len);
int recv_unix_req(int sockfd, char* msg, int msg_len);

#endif
