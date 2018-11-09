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

build: testClient

testClient: testClient.o

tidy:
	rm -f *.jpg ./images/*.jpg

clean:
	rm -f testClient testClient.o

clobber: clean tidy
	rm -f ../bin/testClient

install:
	cp testClient ../bin
