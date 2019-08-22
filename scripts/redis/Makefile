SOURCES = add_redis_command.cpp

OBJECTS = $(SOURCES:.cpp=.o)

BINARIES = add_redis_command

TARGETS += $(BINARIES)

CXX = g++
CXXFLAGS = -g -O3 -std=c++17 -fno-omit-frame-pointer -m64 -W -Wall -Werror \
	-Wswitch-enum -D_GNU_SOURCE=1 \
	-Wundef -fPIC

all: $(TARGETS)

$(OBJECTS): %.o: %.cpp Makefile
	$(CXX) -c $(CXXFLAGS) -o $(@) $(<)

$(BINARIES): %: $(OBJECTS)
	$(CXX) -o $(@) $(^) -pthread

clean:
	rm -f $(OBJECTS) $(TARGETS)

.PHONY: clean
