# ---------------------------------------------------------------------------
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License version 2 as
#  published by the Free Software Foundation.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
#  As a special exception, you may use this file as part of a free software
#  library without restriction.  Specifically, if other files instantiate
#  templates or use macros or inline functions from this file, or you compile
#  this file and link it with other files to produce an executable, this
#  file does not by itself cause the resulting executable to be covered by
#  the GNU General Public License.  This exception does not however
#  invalidate any other reasons why the executable file might be covered by
#  the GNU General Public License.
#
# ---------------------------------------------------------------------------

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
# aggiungere qui altri targets
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

test: $(TARGET)
	./test.sh
