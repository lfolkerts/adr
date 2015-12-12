#https://sites.google.com/site/michaelsafyan/software-engineering/how-to-write-a-makefile
CC := gcc
program_NAMES := arp_3650 tour_3650
testprogram_NAMES := mcast_3650 ssrr_3650 arp_3650 arptest_3650 tour_3650

program_C_SRCS := $(wildcard *.c)
#program_CXX_SRCS := $(wildcard *.cpp)
program_C_OBJS := ${program_C_SRCS:.c=.o}
#program_CXX_OBJS := ${program_CXX_SRCS:.cpp=.o}
program_OBJS := $(program_C_OBJS) #$(program_CXX_OBJS)
MCAST_OBJS :=  mcast_test.o minix.o mcast.o hw_addrs.o
SSRR_OBJS :=  ssrr_test.o minix.o rt.o hw_addrs.o
ARPTEST_OBJS :=  arp_test.o minix.o hw_addrs.o domainsock.o arp.o arpcache.o

TOUR_OBJS :=  tour.o minix.o rt.o hw_addrs.o mcast.o
ARP_OBJS := arp_mod.o minix.o hw_addrs.o domainsock.o arp.o arpcache.o

program_INCLUDE_DIRS :=  /users/cse533/Stevens/unpv13e/lib/
program_LIBRARY_DIRS :=  
program_LIBRARIES := pthread
program_MAGICK := `pkg-config --cflags --libs MagickWand`

CFLAGS = -g  -Wall -Werror -Wno-unused-variable -Wno-unused-function

CPPFLAGS += $(foreach includedir,$(program_INCLUDE_DIRS),-I$(includedir))
LDFLAGS += $(foreach librarydir,$(program_LIBRARY_DIRS),-L$(librarydir))
LDFLAGS += $(foreach library,$(program_LIBRARIES),-l$(library))
LDFLAGS += /users/cse533/Stevens/unpv13e/libunp.a
FLAGS :=  

.PHONY: all clean distclean

all: $(program_NAMES)
test: $(testprogram_NAMES)

tour_3650: $(TOUR_OBJS)
	$(CC) $(FLAGS)  $(CPPFLAGS) $(TOUR_OBJS)  -o tour_3650  $(LDFLAGS)  

arp_3650: $(ARP_OBJS)
	$(CC) $(FLAGS)  $(CPPFLAGS) $(ARP_OBJS)  -o arp_3650  $(LDFLAGS)  


arptest_3650: arp_3650 $(ARPTEST_OBJS)
	$(CC) $(FLAGS)  $(CPPFLAGS) $(ARPTEST_OBJS)  -o arptest_3650  $(LDFLAGS) 

ssrr_3650: $(SSRR_OBJS)
	$(CC) $(FLAGS)  $(CPPFLAGS) $(SSRR_OBJS)  -o ssrr_3650  $(LDFLAGS)  

mcast_3650: $(MCAST_OBJS)
	$(CC) $(FLAGS)  $(CPPFLAGS) $(MCAST_OBJS)  -o mcast_3650  $(LDFLAGS)  

clean:
	@- $(RM) $(testprogram_NAMES)
	@- $(RM) $(program_OBJS)

distclean: clean

