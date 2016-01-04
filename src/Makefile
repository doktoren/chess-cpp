CXX = g++

COMMON_FLAGS = -arch x86_64 -Wall -pedantic -pedantic-errors -ansi -Werror -Wextra -Wcomment -Wformat -Winit-self -Wmissing-include-dirs
CFLAGS  = -O3 -DNDEBUG $(COMMON_FLAGS)
CFLAGS_DB = -ggdb3 -pg $(COMMON_FLAGS)
CFLAGS_XB = -O3 -DNDEBUG -DXBOARD $(COMMON_FLAGS)
CFLAGS_DB_XB = -ggdb3 -pg -DXBOARD $(COMMON_FLAGS)

SOURCES = $(wildcard *.cxx) $(wildcard */*.cxx) $(wildcard */*/*.cxx)
MODS = $(SOURCES:.cxx=.o)

all: chess

chess: $(MODS)
	g++ $(MODS) -o $@

%.o: %.cxx
	$(CXX) $(CFLAGS) -c -o $@ $<

%.d: %.cxx
	g++ -MM $< -o $@

include ${MODS:.o=.d}

clean:
	-rm -f *.o *.d */*.o */*.d
