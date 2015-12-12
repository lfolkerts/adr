CC = gcc
CFLAGS = -g -I/users/cse533/Stevens/unpv13e/lib/ -Wall -Wno-unused-variable -Wno-unused-function
LIBS = /users/cse533/Stevens/unpv13e/libunp.a -lpthread

all: arp sample_tour

arp: arp.o
	${CC} ${CFLAGS} -o arp arp_mod.o	${LIBS}
arp.o: arp_mod.c
	${CC} ${CFLAGS} -c arp_mod.c

sample_tour: sample_tour.o
	${CC} ${CFLAGS} -o sample_tour sample_tour.o	${LIBS}
sample_tour.o: sample_tour.c
	${CC} ${CFLAGS} -c sample_tour.c

clean:
	rm -f *.o tour
