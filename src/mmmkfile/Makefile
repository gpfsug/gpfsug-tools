CC = cc
CC_R = cc
CXX = c++
GPFS_TYPE = GPFS_LINUX

IS64BIT := $(shell getconf LONG_BIT)
ifeq ($(IS64BIT), 64)
LFSDEFINES = -D_FILE_OFFSET_BITS=64 $(shell getconf LFS64_CFLAGS) $(shell getconf LFS64_LINTFLAGS) -m64
else
LFSDEFINES = $(shell getconf LFS_CFLAGS) $(shell getconf LFS_LINTFLAGS) -m32
endif

LIB = -lgpfs 
OTHERINCL=
OTHERLIB= -lpthread

CFLAGS = -O -D$(GPFS_TYPE) $(LFSDEFINES)

PROGS = mmmkfile 

all: $(PROGS)
debug: CXX += -DDEBUG -g
debug: CC += -DDEBUG -g
debug: $(PROGS)

clean: 
	rm -f $(PROGS)

mmmkfile: mmmkfile.C
	$(CXX) -o mmmkfile $(CFLAGS) $(OTHERINCL) $(OTHERLIB) $(LIB) mmmkfile.C

test: test.C
	$(CXX) -o test $(CFLAGS) $(OTHERINCL) $(OTHERLIB) $(LIB) test.C

test2: test2.c
	$(CXX) -o test2 $(CFLAGS) $(OTHERINCL) $(OTHERLIB) $(LIB) test2.c
