#include "unp.h"
#include "unpifi.h"
#include <assert.h>
struct hwa_info * get_hw_addrs();
void free_hwa_info(struct hwa_info *hwahead);
struct vm_iface_info *get_vm_iface_info(void);
void print_vm_iface_info(struct vm_iface_info *vm);
void prhwaddrs (void);

