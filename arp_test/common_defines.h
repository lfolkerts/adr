#include "unp.h"
#include "unpifi.h"
#include "hw_addrs.h"
#include <assert.h>
#include <net/ethernet.h>
#include <netpacket/packet.h>
#include <net/if_arp.h>
#include <netinet/ip.h>
#define TOUR_PROTO (109)
#define TOUR_ID 31337
#define TOUR_PROTOID 0x3133
#define MCAST_ADDR "239.255.107.49"
#define MCAST_PORT "5324"
#define MCAST_PORT_INT 5324
#define IP4_HDRLEN 20
#define NUM_CACHE_ENTRIES 10
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

#define MAX_TOUR_LEN 32
#define VM1_ADDR  "130.245.156.21"
#define VM2_ADDR  "130.245.156.22"
#define VM3_ADDR  "130.245.156.23"
#define VM4_ADDR  "130.245.156.24"
#define VM5_ADDR  "130.245.156.25"
#define VM6_ADDR  "130.245.156.26"
#define VM7_ADDR  "130.245.156.27"
#define VM8_ADDR  "130.245.156.28"
#define VM9_ADDR  "130.245.156.29"
#define VM10_ADDR "130.245.156.20"

#define ARP_PATH "/tmp/arp_3524"
#define ARRAY_SIZE(name) sizeof(name)/sizeof(name[0])
struct gen_ll {
	void *data;
	struct gen_ll *next;
	struct gen_ll *prev;
};

struct ll_info {
	int sockfd;
	int ifi_idx;
	char mac_addr[IF_HADDR];
};
struct tour_socket_info {
	int rt_socket;
	int pg_socket;
	int mcast_socket_send;
	int mcast_socket_recv;
	struct ll_info *ll;
};

struct vm_iface_info {
	int if_index;
	uint8_t if_haddr[IF_HADDR];
	char ip_addr[INET_ADDRSTRLEN];
	struct vm_iface_info *next;
};

struct ethernet_hdr {
	uint8_t dst_mac[IF_HADDR];
	uint8_t src_mac[IF_HADDR];
	uint16_t type_id;
};

struct tour_pkt {
	struct ip ip_hdr;
	int index;
	int end_index;
	char ip_addrs[MAX_TOUR_LEN][INET_ADDRSTRLEN];
};

struct arp_packet {
	char src_ip[INET_ADDRSTRLEN];
	uint8_t src_mac[IF_HADDR];	
	char target_ip[INET_ADDRSTRLEN];
	uint8_t target_mac[IF_HADDR];	
};

struct ethernet_frame {
	struct ethernet_hdr eh;
	struct arp_packet arp_pkt;
};

struct arp_cache_entry {
	char ip[INET_ADDRSTRLEN];
	int ifi_idx;
	uint8_t mac_addr[IF_HADDR];
};
struct vm_addr_struct {
	int idx ;
	char addr[32];
};

struct vm_iface_info *if_head;
struct vm_addr_struct vm[] = {
	{.idx = VM1_IDX,  .addr = VM1_ADDR},
	{.idx = VM2_IDX,  .addr = VM2_ADDR},
	{.idx = VM3_IDX,  .addr = VM3_ADDR},
	{.idx = VM4_IDX,  .addr = VM4_ADDR},
	{.idx = VM5_IDX,  .addr = VM5_ADDR},
	{.idx = VM6_IDX,  .addr = VM6_ADDR},
	{.idx = VM7_IDX,  .addr = VM7_ADDR},
	{.idx = VM8_IDX,  .addr = VM8_ADDR},
	{.idx = VM9_IDX,  .addr = VM9_ADDR},
	{.idx = VM10_IDX, .addr = VM10_ADDR},
};

char *can_ip;
void gen_ll_init(struct gen_ll **head)
{
	*head = Malloc(sizeof(*head));
	(*head)->prev = (*head)->next = *head;
	return;
}

void gen_ll_add(struct gen_ll *head, void *data, size_t len)
{
	struct gen_ll *node = Calloc(1, sizeof(*node));
	node->data = Calloc(1, len);
	memcpy(node->data, data, len);

	struct gen_ll *bk;
	bk = head->prev;
	node->prev = bk;
	node->next = head;
	head->prev = node;
	bk->next = node;
}

	struct hwa_info *
get_hw_addrs()
{
	struct hwa_info	*hwa, *hwahead, **hwapnext;
	int		sockfd, len, lastlen, alias, nInterfaces, i;
	char		*ptr, *buf, lastname[IF_NAME], *cptr;
	struct ifconf	ifc;
	struct ifreq	*ifr, *item, ifrcopy;
	struct sockaddr	*sinptr;

	sockfd = Socket(AF_INET, SOCK_DGRAM, 0);

	lastlen = 0;
	len = 100 * sizeof(struct ifreq);	/* initial buffer size guess */
	for ( ; ; ) {
		buf = (char*) Malloc(len);
		ifc.ifc_len = len;
		ifc.ifc_buf = buf;
		if (ioctl(sockfd, SIOCGIFCONF, &ifc) < 0) {
			if (errno != EINVAL || lastlen != 0)
				err_sys("ioctl error");
		} else {
			if (ifc.ifc_len == lastlen)
				break;		/* success, len has not changed */
			lastlen = ifc.ifc_len;
		}
		len += 10 * sizeof(struct ifreq);	/* increment */
		free(buf);
	}

	hwahead = NULL;
	hwapnext = &hwahead;
	lastname[0] = 0;

	ifr = ifc.ifc_req;
	nInterfaces = ifc.ifc_len / sizeof(struct ifreq);
	for(i = 0; i < nInterfaces; i++)  {
		item = &ifr[i];
		alias = 0; 
		hwa = (struct hwa_info *) Calloc(1, sizeof(struct hwa_info));
		memcpy(hwa->if_name, item->ifr_name, IF_NAME);		/* interface name */
		hwa->if_name[IF_NAME-1] = '\0';
		/* start to check if alias address */
		if ( (cptr = (char *) strchr(item->ifr_name, ':')) != NULL)
			*cptr = 0;		/* replace colon will null */
		if (strncmp(lastname, item->ifr_name, IF_NAME) == 0) {
			alias = IP_ALIAS;
		}
		memcpy(lastname, item->ifr_name, IF_NAME);
		ifrcopy = *item;
		*hwapnext = hwa;		/* prev points to this new one */
		hwapnext = &hwa->hwa_next;	/* pointer to next one goes here */

		hwa->ip_alias = alias;		/* alias IP address flag: 0 if no; 1 if yes */
		sinptr = &item->ifr_addr;
		hwa->ip_addr = (struct sockaddr *) Calloc(1, sizeof(struct sockaddr));
		memcpy(hwa->ip_addr, sinptr, sizeof(struct sockaddr));	/* IP address */
		if (ioctl(sockfd, SIOCGIFHWADDR, &ifrcopy) < 0)
			perror("SIOCGIFHWADDR");	/* get hw address */
		memcpy(hwa->if_haddr, ifrcopy.ifr_hwaddr.sa_data, IF_HADDR);
		if (ioctl(sockfd, SIOCGIFINDEX, &ifrcopy) < 0)
			perror("SIOCGIFINDEX");	/* get interface index */
		memcpy(&hwa->if_index, &ifrcopy.ifr_ifindex, sizeof(int));
	}
	free(buf);
	return(hwahead);	/* pointer to first structure in linked list */
}

	void
free_hwa_info(struct hwa_info *hwahead)
{
	struct hwa_info	*hwa, *hwanext;

	for (hwa = hwahead; hwa != NULL; hwa = hwanext) {
		free(hwa->ip_addr);
		hwanext = hwa->hwa_next;	/* can't fetch hwa_next after free() */
		free(hwa);			/* the hwa_info{} itself */
	}
}
/* end free_hwa_info */

	struct hwa_info *
Get_hw_addrs()
{
	struct hwa_info	*hwa;

	if ( (hwa = get_hw_addrs()) == NULL)
		err_quit("get_hw_addrs error");
	return(hwa);
}

struct vm_iface_info *get_vm_iface_info(void)
{
	struct hwa_info *hwa;
	struct vm_iface_info *vm_head = NULL, **vm_pnext = &vm_head, *vm_node;

	for (hwa = Get_hw_addrs(); hwa != NULL; hwa = hwa->hwa_next) {
		if (strcmp(hwa->if_name, "eth0") != 0)
			continue;
		vm_node = Calloc(1, sizeof(*vm_node));
		vm_node->if_index = hwa->if_index;
		strcpy(vm_node->ip_addr, Sock_ntop_host(hwa->ip_addr, sizeof(*(hwa->ip_addr))));
		memcpy(vm_node->if_haddr, hwa->if_haddr, sizeof(hwa->if_haddr));
		*vm_pnext = vm_node;
		vm_pnext = &vm_node->next;
	}
	return vm_head;
}


struct ll_info * bind_link_socket(struct vm_iface_info *vm, int type)
{
	struct sockaddr_ll l_addr;
	struct vm_iface_info *t = vm;
	struct ll_info *ll = Calloc(1, sizeof(*ll));
	memset(&l_addr, 0x0, sizeof(l_addr));

	while (t) {
		ll->sockfd = Socket(PF_PACKET, SOCK_RAW, htons(type));
		ll->ifi_idx = t->if_index;
		l_addr.sll_family = PF_PACKET;
		l_addr.sll_ifindex = t->if_index;
		l_addr.sll_hatype = ARPHRD_ETHER;
		l_addr.sll_pkttype = PACKET_OTHERHOST;
		l_addr.sll_halen = ETH_ALEN;
		memcpy(l_addr.sll_addr, t->if_haddr, sizeof(t->if_haddr));
		memcpy(ll->mac_addr, t->if_haddr, sizeof(t->if_haddr));
		/* unused bytes set to 0, by memset() before */
		Bind(ll->sockfd, (void *) &l_addr, sizeof(l_addr));
		t = t->next;
	}
	return ll;
}

void gen_ll_add_head(struct gen_ll *head, void *data, size_t len)
{
	struct gen_ll *node = Calloc(1, sizeof(*node));
	node->data = Calloc(1, len);
	memcpy(node->data, data, len);

	struct gen_ll *fd;
	fd = head->next;
	node->next = fd;
	node->prev = head;
	head->next = node;
	fd->prev = node;
}
void gen_ll_del(struct gen_ll *head, void *data, size_t len)
{
	struct gen_ll *node = head->next;

	while (node != head) {
		if (memcmp(node->data, data, len) == 0) {
			struct gen_ll *bk, *fd;

			bk = node->prev;
			fd = node->next;
			fd->prev = bk;
			bk->next = fd;
			free(node->data);
			free(node);
			return;
		}
		node = node->next;
	}
}

void *gen_ll_lookup(struct gen_ll *head, void *data, size_t len)
{

	struct gen_ll *node = head->next;

	while (node != head) {
		if (memcmp(node->data, data, len) == 0) {
			return node->data;
		}
		node = node->next;
	}
	return NULL;
}

void usage(char *prog_name)
{
	fprintf(stderr, "Usage: %s <list of vms to tour>\n", prog_name);
	exit(-1);
}


static char *get_canonical_ip(char *name)
{
	/* skip the "vm" part */
	char *idx = &name[2];
	int index, i;

	index = atoi(idx);
	assert(index > 0 && index <= 10);
	/* array is zero based */
	index -= 1;
	for (i = 0; i < ARRAY_SIZE(vm); i++) {
		if (vm[i].idx == index)
			return vm[i].addr;
	}
	return NULL;
}

static void get_lhostname(char *buf, size_t len)
{
	int ret;

	/* get hostname doesn't guarantee null termination */
	buf[len -1] = 0x0;
	ret = gethostname(buf, len - 1);
	if (ret == -1) {
		printf("error in gethostname\n");
		exit(-1);
	}
	return ;
}

static uint16_t get_eph_port()
{
	/* we rely on randomness to make sure we get a unique eph port */
	return (uint16_t ) rand();
}

static int _select(int nfds, fd_set *rset, fd_set *wset, fd_set *eset, struct timeval *tv)
{
	int ret;
again:
	if ( (ret = select(nfds, rset, wset, eset, tv)) < 0) {
		if (errno == EINTR)
			goto again;
		else
			err_sys("Select error\n");
	}
	return ret;
}

static char *get_sunpath(int sockfd)
{
	struct sockaddr_un *s = Calloc(1, sizeof(*s));
	socklen_t len = sizeof(*s);
	Getsockname(sockfd, (void *)s, &len);
	return s->sun_path;
}

int get_num_entries(struct gen_ll *head)
{
	struct gen_ll *node = head->next;
	int count = 0;

	while (node != head) {
		count ++;
		node = node->next;
	}
	return count;
}

void get_vm_info(struct gen_ll *vm_head, char **argv, int argc)
{

	int i;
	char *ip_addr;

	for (i = 1; i < argc; i++) {
		/* at first we dont do any sanity checking and just add all the vm addr to our list */
		ip_addr = get_canonical_ip(argv[i]);
		gen_ll_add(vm_head, ip_addr, strlen(ip_addr) + 1);	
	}

	char lhostname[INET_ADDRSTRLEN];
	get_lhostname(lhostname, sizeof(lhostname));
	ip_addr = get_canonical_ip(lhostname);
	gen_ll_add_head(vm_head, ip_addr, strlen(ip_addr) + 1);
	/* make sure to add the source node to the head of the list */

	/* the first check is to make sure that 2 adjacent vm addr are not the same */
	struct gen_ll *node = vm_head->next;
	int count = get_num_entries(vm_head);
	struct gen_ll *curr, *next;

	for (i = 1; i < count; i++) {
		curr = node;
		next = curr->next;
		if (strcmp( (char *)curr->data, (char *)next->data ) == 0) {
			/* have to update the node before deleting, since after delete we no longer have 
			   a reference to its memory */
			node = node->next;
			gen_ll_del(vm_head, curr->data, strlen((char *) curr->data) + 1);
			continue;
		}
		node = node->next;
	}
}

void print_vm_info(struct gen_ll *head)
{
	struct gen_ll *node = head->next;
	printf("VM addr : \n");
	while (node != head) {
		printf("%s\n", (char *) node->data);
		node = node->next;
	}
}

void initialize_sockets(struct tour_socket_info *ts)
{
	int enable = 1;
	ts->rt_socket = Socket(AF_INET, SOCK_RAW, TOUR_PROTO);
	Setsockopt(ts->rt_socket, IPPROTO_IP, IP_HDRINCL, &enable, sizeof(enable));
	ts->pg_socket = Socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	ts->ll = bind_link_socket(if_head, ETH_P_IP);
}

void initialize_mcast_socket(struct tour_socket_info *ts)
{
	struct sockaddr_in recv;
	ts->mcast_socket_send = Socket(AF_INET, SOCK_DGRAM, 0); 
	ts->mcast_socket_recv = Socket(AF_INET, SOCK_DGRAM, 0);

	memset(&recv, 0x0, sizeof(recv));

	recv.sin_family = AF_INET;
	recv.sin_port = htons(MCAST_PORT_INT);
	Inet_pton(AF_INET, MCAST_ADDR, &recv.sin_addr);
	Bind(ts->mcast_socket_recv, (void *) &recv, sizeof(recv));
	Mcast_join(ts->mcast_socket_recv, (void *) &recv, sizeof(recv), "eth0", 0);
	Mcast_set_loop(ts->mcast_socket_send, 0);
}


static void construct_tour_pktdata(struct tour_pkt *pkt, struct gen_ll *head)
{
	struct gen_ll *node = head->next;
	int i = 0;

	while (node != head) {
		memcpy(pkt->ip_addrs[i], node->data, INET_ADDRSTRLEN);
		node = node->next;
		i ++;
	}
	pkt->index = htonl(1);
	pkt->end_index = htonl(i);

}

static void construct_ip_hdr(struct tour_pkt *pkt, char *src_ip, char *dst_ip)
{
	struct ip *iphdr = &pkt->ip_hdr;
	/* code borrowed from www.pdbuchan.com/rawsock/icmp4_ll.c */
	iphdr->ip_hl = sizeof(struct ip) >> 2;
	iphdr->ip_v = IPVERSION;
	iphdr->ip_tos = 0;
	iphdr->ip_len = htons (sizeof(*pkt));
	iphdr->ip_id = htons (TOUR_ID);
	iphdr->ip_off = 0;
	iphdr->ip_ttl = 255;
	iphdr->ip_p = (uint8_t)TOUR_PROTO;
	Inet_pton(AF_INET, src_ip, &iphdr->ip_src);
	Inet_pton(AF_INET, dst_ip, &iphdr->ip_dst);
	iphdr->ip_sum = htons(in_cksum((void *) pkt, sizeof(*pkt)));

}

static int construct_tour_pkthdr(struct tour_pkt *pkt)
{
	int index = ntohl(pkt->index);
	int end_index = ntohl(pkt->end_index);
	if (index == end_index)
		return -1;
	char *src_ip = can_ip;
	char *dst_ip = pkt->ip_addrs[index];
	index ++;
	pkt->index = htonl(index);
	construct_ip_hdr(pkt, src_ip, dst_ip);
	return 0;
}

struct tour_pkt *construct_tour_packet(struct gen_ll *head) 
{
	int ret;
	struct tour_pkt *pkt = Calloc(1, sizeof(*pkt));

	construct_tour_pktdata(pkt, head);
	ret = construct_tour_pkthdr(pkt);
	if (ret < 0) 
		return NULL;

	return pkt;
}

void init_tour()
{

	char lhostname[INET_ADDRSTRLEN];
	socklen_t len;

	get_lhostname(lhostname, sizeof(lhostname));
	can_ip = get_canonical_ip(lhostname);
	if_head = get_vm_iface_info();
}
static char *get_hostname(struct in_addr *addr)
{
	struct hostent *h;

	h = gethostbyaddr((void *) addr, sizeof(*addr), AF_INET);
	return h->h_name;
}

void send_ping_request(int sockfd, char *can_ip, struct in_addr *dst_ip)
{
	return;
}

int bind_domain_socket(char *path)
{
	struct sockaddr_un addr;
	int sockfd = Socket(AF_LOCAL, SOCK_STREAM, 0 );

	memset(&addr, 0x0, sizeof(addr));
	unlink(path);
	addr.sun_family = AF_LOCAL;
	strcpy(addr.sun_path, path);
	Bind(sockfd, (void *) &addr, sizeof(addr));
	Listen(sockfd, LISTENQ);
	return sockfd;
}

struct arp_cache_entry *c_head;

void init_arp_cache(int num)
{
	c_head = Calloc(1, num * sizeof(*c_head));
}

struct arp_cache_entry *lookup_arp_cache(char *ip)
{
	int i;

	for (i = 0; i < NUM_CACHE_ENTRIES; i++) {
		if (strcmp(c_head[i].ip , ip) == 0) {
			return &c_head[i];	
		}
	}
	return NULL;	
}

void write_frame(struct ethernet_frame *eframe, struct ll_info *ll)
{
	int i;
	struct sockaddr_ll addr;
	struct ethernet_hdr *ehdr = (void *)eframe;
	int sockfd = -1;
	socklen_t len = sizeof(addr);

	memset(&addr, 0x0, sizeof(addr));

	addr.sll_pkttype = PACKET_OTHERHOST;

	addr.sll_family = PF_PACKET;
	addr.sll_ifindex = ll->ifi_idx;
	addr.sll_hatype = ARPHRD_ETHER;
	addr.sll_halen = ETH_ALEN;
	memcpy(addr.sll_addr, ehdr->dst_mac, IF_HADDR);
	sockfd = ll->sockfd;
	Sendto(sockfd, eframe, sizeof(*eframe), 0 , (void *) &addr, len);
	return;
}

void add_to_arp_cache(struct arp_packet *pkt, struct sockaddr_ll *addr)
{
	int i;

	if (lookup_arp_cache(pkt->src_ip) == NULL) {
		for (i = 0; i < NUM_CACHE_ENTRIES; i++) {
			if (strlen(c_head[i].ip) == 0) {
				strcpy(c_head[i].ip, pkt->src_ip);
				memcpy(c_head[i].mac_addr, pkt->src_mac, IF_HADDR);
				c_head[i].ifi_idx = addr->sll_ifindex;
				return;
			}
		}
	}
}

uint8_t *handle_arp_request(char *ip, struct ll_info *info) 
{
	struct ethernet_frame frame;
	struct ethernet_hdr *e = &frame.eh;
	struct arp_packet *arp = &frame.arp_pkt;
	int i;
	uint8_t *mac = Calloc (1, IF_HADDR);

	memset(&frame, 0x0, sizeof(frame));

	memcpy(e->src_mac, info->mac_addr, IF_HADDR);
	for (i = 0; i < IF_HADDR; i++) {
		e->dst_mac[i] = 0xFF;
	}
	e->type_id = htons(TOUR_PROTOID);

	memcpy(arp->src_mac, info->mac_addr, IF_HADDR);
	strcpy(arp->src_ip, can_ip);
	strcpy(arp->target_ip, ip);
	
	write_frame(&frame, info);
	
	memset(&frame, 0x0, sizeof(frame));
	struct sockaddr_ll addr;
	memset( &addr, 0x0, sizeof(addr));
	socklen_t len = sizeof(addr);

	Recvfrom(info->sockfd, &frame, sizeof(frame), 0, (void *) &addr, &len);

	add_to_arp_cache(&frame.arp_pkt, &addr);
	struct arp_packet *p = &frame.arp_pkt;

	memcpy(mac, p->src_mac, IF_HADDR);
	return mac;
}

void handle_frame(struct ethernet_frame *frame, struct ll_info *ll)
{
	struct arp_packet *arp = &frame->arp_pkt;
	struct ethernet_hdr *eh = &frame->eh;

	if (strcmp(can_ip, arp->target_ip) == 0) {
		/* send an ARP reply back */

		struct ethernet_frame rframe;

		memset(&rframe, 0x0, sizeof(rframe));

		struct ethernet_hdr *e = &rframe.eh;
		struct arp_packet *apkt = &rframe.arp_pkt;
		int i;
		char *mac = Calloc (1, IF_HADDR);

		memcpy(e->src_mac, ll->mac_addr, IF_HADDR);
		memcpy(e->dst_mac, eh->src_mac, IF_HADDR);
		e->type_id = htons(TOUR_PROTOID);
		strcpy(apkt->src_ip, can_ip);
		memcpy(apkt->src_mac, ll->mac_addr, IF_HADDR);
		write_frame(&rframe, ll);
	}
	return;
}
