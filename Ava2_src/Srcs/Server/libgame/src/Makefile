CXX	= g++11
BIN_DIR = ../lib
BIN = $(BIN_DIR)/libgame.a

GCC_VERSION = $(shell $(CC) --version 2>&1 | grep "(GCC)" | cut -d' ' -f3  | cut -d'.' -f1)

#ifeq ($(GCC_VERSION), 4)
#CFLAGS = -Wall -O2 -pipe -mtune=i686 -fno-exceptions -I../include
#else
#CFLAGS = -Wall -O2 -pipe -mcpu=i686 -fno-exceptions -I../include
#endif
CFLAGS = -m32 -Wall -O2 -pipe -fno-exceptions -I../include
#CFLAGS =  -Wall -O2 -pipe -fno-exceptions -I../include
CFLAGS += -std=c++14

LIBS =

OBJFILES = grid.o attribute.o targa.o

default:
	$(MAKE) $(BIN)

$(BIN): $(OBJFILES)
	if [ ! -d $(BIN_DIR) ]; then mkdir $(BIN_DIR); fi
	ar cru $(BIN) $(OBJFILES) $(LIBS)
	ranlib $(BIN)
	chmod 700 $(BIN)

clean:
	rm -f *.o
	rm -f $(BIN)

dep:
	touch Depend
	$(CXX) $(CFLAGS) -MM *.cc > Depend

$(OBJFILES):
	$(CXX) $(CFLAGS) -c $<

include Depend

