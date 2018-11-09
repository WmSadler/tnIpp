CXXFLAGS = `pkg-config --cflags opencv`
CXXFLAGS += -I../include
CXXFLAGS +=-ggdb
CXXFLAGS +=-fexceptions
CXXFLAGS +=-O0

LDLIBS = `pkg-config --libs opencv`
LDLIBS += -lstdc++

all: build

build: vidTestClient

testClient: vidTestClient.o

tidy:

clean:
	rm -f vidTestClient vidTestClient.o

clobber: clean tidy
	rm -f ../bin/vidTestClient

install:
	cp vidTestClient ../bin
