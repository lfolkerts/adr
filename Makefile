#https://sites.google.com/site/michaelsafyan/software-engineering/how-to-write-a-makefile
CC := gcc
program_NAME := mcast_3560
program_C_SRCS := $(wildcard *.c)
#program_CXX_SRCS := $(wildcard *.cpp)
program_C_OBJS := ${program_C_SRCS:.c=.o}
#program_CXX_OBJS := ${program_CXX_SRCS:.cpp=.o}
program_OBJS := $(program_C_OBJS) #$(program_CXX_OBJS)
program_INCLUDE_DIRS :=  /users/cse533/Stevens/unpv13e/lib/
program_LIBRARY_DIRS :=  
program_LIBRARIES := pthread
program_MAGICK := `pkg-config --cflags --libs MagickWand`

CFLAGS = -g  -Wall -Wno-unused-variable -Wno-unused-function

CPPFLAGS += $(foreach includedir,$(program_INCLUDE_DIRS),-I$(includedir))
LDFLAGS += $(foreach librarydir,$(program_LIBRARY_DIRS),-L$(librarydir))
LDFLAGS += $(foreach library,$(program_LIBRARIES),-l$(library))
LDFLAGS += /users/cse533/Stevens/unpv13e/libunp.a
FLAGS :=  

.PHONY: all clean distclean

all: $(program_NAME)

$(program_NAME): $(program_OBJS)
	$(CC) $(FLAGS)  $(CPPFLAGS) $(program_OBJS)  -o $(program_NAME)  $(LDFLAGS)  

clean:
	@- $(RM) $(program_NAME)
	@- $(RM) $(program_OBJS)

distclean: clean
