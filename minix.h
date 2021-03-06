#ifndef __MINIX_H__
#define __MINIX_H__

#include <stdint.h>
#define SERVER_PORT 31337
#define SERVER_SUNPATH "/tmp/odrserver_3254"


enum {
        VM1_IDX,
        VM2_IDX,
        VM3_IDX,
        VM4_IDX,
        VM5_IDX,
        VM6_IDX,
        VM7_IDX,
        VM8_IDX,
        VM9_IDX,
        VM10_IDX,
};

#define VM1_ADDR   "130.245.156.21"
#define VM2_ADDR   "130.245.156.22"
#define VM3_ADDR   "130.245.156.23"
#define VM4_ADDR   "130.245.156.24"
#define VM5_ADDR   "130.245.156.25"
#define VM6_ADDR   "130.245.156.26"
#define VM7_ADDR   "130.245.156.27"
#define VM8_ADDR   "130.245.156.28"
#define VM9_ADDR   "130.245.156.29"
#define VM10_ADDR  "130.245.156.20"
#define MCAST_ADDR "239.255.107.49"

struct vm_addr_struct {
        int idx ;
        char addr[32];
};

char *get_canonical_ip(char *name);
void get_lhostname(char *buf, size_t len);
uint16_t get_eph_port();
int _select(int nfds, fd_set *rset, fd_set *wset, fd_set *eset, struct timeval *tv);
static char *get_sunpath(int sockfd);
char *get_vm_name(uint32_t ip4);

#endif

