CXX = g++

MAKEFLAGS+="-j $(shell grep -c ^processor /proc/cpuinfo) "

COMMON_FLAGS = -Wall -pedantic -pedantic-errors -ansi -Werror -Wextra -Wcomment -Wformat -Winit-self -Wmissing-include-dirs -Wno-unused-result
CFLAGS__  = -O3 -DNDEBUG $(COMMON_FLAGS)
CFLAGS_DB_ = -ggdb3 -pg $(COMMON_FLAGS)
CFLAGS__XB = -O3 -DNDEBUG -DXBOARD $(COMMON_FLAGS)
CFLAGS_DB_XB = -ggdb3 -pg -DXBOARD $(COMMON_FLAGS)

SOURCES = $(wildcard *.cxx) $(wildcard */*.cxx) $(wildcard */*/*.cxx)
MODS = $(SOURCES:.cxx=.o)

all: chess

chess: $(MODS)
	g++ $(MODS) -o $@

%.o: %.cxx
	$(CXX) $(CFLAGS_$(DB)_$(XB)) -c -o $@ $<

%.d: %.cxx
	g++ -MM $< -o $@

include ${MODS:.o=.d}

clean:
	-rm -f *.o *.d */*.o */*.d */*/*.o */*/*.d
