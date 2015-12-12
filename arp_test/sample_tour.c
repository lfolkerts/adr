
#include "common_defines.h"


int main(int argc, char **argv)
{
	if (argc != 2) {
		printf("Usage: %s <vm name>\n", argv[0]);
		exit(-1);
	}
	int sockfd;
	uint8_t mac[6];
	char *ip = get_canonical_ip(argv[1]);

	sockfd = Socket(AF_LOCAL, SOCK_STREAM, 0);
	struct sockaddr_un serv_addr;

	memset(&serv_addr, 0x0, sizeof(serv_addr));
	strcpy(serv_addr.sun_path, "/tmp/arp_3524");
	serv_addr.sun_family = AF_LOCAL;

	Connect(sockfd, (void *) &serv_addr, sizeof(serv_addr));
	Write(sockfd, ip, strlen(ip) + 1);
	Read(sockfd, mac, 6);

	int i;
	printf("MAC addr for %s is \n", ip); 
	for (i = 0;i < 6; i++) {
		printf("%02x:", mac[i]);
	}
	printf("\n");
	return 0;
}
