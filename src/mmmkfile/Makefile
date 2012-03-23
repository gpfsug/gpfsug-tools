CC = cc
CC_R = cc
CXX = c++
GPFS_TYPE = GPFS_LINUX
LIB = -lgpfs 
OTHERINCL=
OTHERLIB= -lpthread

CFLAGS = -O -D$(GPFS_TYPE) 

PROGS = mmmkfile
#PROGS = test

all: $(PROGS)
debug: CXX += -DDEBUG -g
debug: CC += -DDEBUG -g
debug: $(PROGS)

clean: 
	rm -f $(PROGS)

mmmkfile: mmmkfile.C
	$(CXX) -o mmmkfile $(CFLAGS) $(OTHERINCL) $(OTHERLIB) $(LIB) mmmkfile.C

#test: test.C
#	$(CXX) -o test $(CFLAGS) $(OTHERINCL) $(OTHERLIB) $(LIB) test.C
