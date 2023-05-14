CC		=  gcc
AR              =  ar
CFLAGS	        += -std=c99 -Wall -ggdb3 -g
ARFLAGS         =  rvs
INCLUDES	= -I. -I ./utils/includes
LDFLAGS 	= -L.
OPTFLAGS	= -O3 -DNDEBUG
LIBS            =  -pthread
SRC_PATH = ./src/
BIN_PATH = ./bin/
LIB_PATH = ./libs/
INCLUDE_PATH = ./includes/
TARGETS		= farm generafile



.PHONY: all clean cleanall
.SUFFIXES: .c .h

%: $(SRC_PATH)%.c
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -o $@ $< $(LDFLAGS) $(LIBS) -Wstringop-truncation


$(BIN_PATH)%.o: $(SRC_PATH)%.c
	$(CC) $(CFLAGS) $(INCLUDES) $(OPTFLAGS) -c -o $@ $<  $(LIBS) -Wno-stringop-truncation

all	: $(TARGETS)

farm: $(BIN_PATH)farm.o $(BIN_PATH)Worker.o $(BIN_PATH)Master.o $(BIN_PATH)Collector.o $(LIB_PATH)libBQueue.a
	$(CC) $(CCFLAGS) $(INCLUDES) $(OPTFLAGS) $(LDFLAGS) -o $@ $^ $(LIBS)

$(LIB_PATH)libBQueue.a: $(BIN_PATH)Queue.o $(INCLUDE_PATH)Queue.h
	$(AR) $(ARFLAGS) $@ $<


$(BIN_PATH)Worker.o: $(SRC_PATH)Worker.c

$(BIN_PATH)Master.o: $(SRC_PATH)Master.c

$(BIN_PATH)Collector.o: $(SRC_PATH)Collector.c

clean		:
	rm -f $(TARGETS)

cleanall	: clean
	rm -f -r testdir
	rm -f *.dat
	rm -f *.txt
	rm -f *.o *~ *.a
	rm -f $(LIB_PATH)*.a
	rm -f $(BIN_PATH)*.o

test: $(TARGETS)
	./test.sh
