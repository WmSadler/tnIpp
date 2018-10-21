CXXFLAGS = `pkg-config --cflags opencv`
CXXFLAGS += `pkg-config --cflags libzmq`
CXXFLAGS += -I../include
CXXFLAGS +=-ggdb
CXXFLAGS +=-fexceptions
CXXFLAGS +=-O0

LDLIBS = `pkg-config --libs opencv`
LDLIBS += `pkg-config --libs libzmq`
LDLIBS += -lstdc++

all: build

build: eTestSvr

testClient: eTestSvr.o

tidy:

clean:
	rm -f eTestSvr eTestSvr.o

clobber: clean tidy
	rm -f ../bin/eTestSvr

install:
	cp eTestSvr ../bin
