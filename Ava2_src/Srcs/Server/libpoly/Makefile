CXX	= g++11
BIN = ./libpoly.a

GCC_VERSION = $(shell $(CC) --version 2>&1 | grep "(GCC)" | cut -d' ' -f3  | cut -d'.' -f1)

#ifeq ($(GCC_VERSION), 4)
#CFLAGS  = -Wall -O2 -pipe -mtune=i686 -fno-exceptions -fno-rtti
#else
#CFLAGS  = -Wall -O2 -pipe -mcpu=i686 -fno-exceptions -fno-rtti
#endif
CFLAGS  = -m32 -Wall -O2 -pipe -fno-exceptions -fno-rtti
#CFLAGS  = -Wall -O2 -pipe -fno-exceptions -fno-rtti
CFLAGS += -std=c++14

LIBS =

OBJFILES = Base.o Poly.o SymTable.o Symbol.o

default:
	$(MAKE) $(BIN)

$(BIN): $(OBJFILES)
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

