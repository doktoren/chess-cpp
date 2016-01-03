CXX = g++

CFLAGS  = -O3 -arch x86_64 -Wall -pedantic -ansi -Werror -Wextra -DNDEBUG
CFLAGS_DB = -ggdb3 -pg -Wall -pedantic -ansi -Werror -Wextra
CFLAGS_XB = -O3 -Wall -pedantic -ansi -DNDEBUG -DXBOARD
CFLAGS_DB_XB = -ggdb3 -pg -Wall -pedantic -ansi -DXBOARD

SOURCES = $(wildcard *.cxx) $(wildcard */*.cxx) $(wildcard */*/*.cxx)
MODS = $(SOURCES:.cxx=.o)

all: chess

chess: $(MODS)
	g++ $(MODS) -o $@

%.o: %.cxx
	$(CXX) $(CFLAGS_DB) -c -o $@ $<

%.d: %.cxx
	g++ -MM $< -o $@

include ${MODS:.o=.d}

clean:
	-rm -f *.o *.d */*.o */*.d
