#ifndef HW_ADDR_H__
#define HW_ADDR_H__


#define IF_NAME         16      /* same as IFNAMSIZ    in <net/if.h> */
#define IF_HADDR         6      /* same as IFHWADDRLEN in <net/if.h> */

#define IP_ALIAS         1      /* hwa_addr is an alias */


/* contains information about the interface other than eth0 and lo */
struct vm_iface_info {
        int if_index;
        uint8_t if_haddr[IF_HADDR];
        char ip_addr[INET_ADDRSTRLEN];
        struct vm_iface_info *next;
};


struct hwa_info {
  char    if_name[IF_NAME];     /* interface name, null terminated */
  char    if_haddr[IF_HADDR];   /* hardware address */
  int     if_index;             /* interface index */
  short   ip_alias;             /* 1 if hwa_addr is an alias IP address */
  struct  sockaddr  *ip_addr;   /* IP address */
  struct  hwa_info  *hwa_next;  /* next of these structures */
};

struct hwa_info * get_hw_addrs();
void free_hwa_info(struct hwa_info *hwahead);
struct vm_iface_info *get_vm_iface_info(void);
void print_vm_iface_info(struct vm_iface_info *vm_iface);
void prhwaddrs (void);

#endif
