CXXFLAGS += `pkg-config --cflags libczmq`
CXXFLAGS += -I../include
CXXFLAGS += -std=c++11
CXXFLAGS += -ggdb
CXXFLAGS += -fexceptions
CXXFLAGS += -O0

LDLIBS = `pkg-config --libs libczmq`
LDLIBS += -lstdc++

all: build

build: ippGpuBroker

tidy:

clean:
	rm -f ippGpuBroker ippGpuBroker.o

clobber: clean
