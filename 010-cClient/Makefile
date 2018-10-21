CFLAGS = `pkg-config --cflags opencv`
CFLAGS += `pkg-config --cflags libzmq`
CFLAGS +=-ggdb

LDLIBS = `pkg-config --libs opencv`
LDLIBS += `pkg-config --libs libzmq`

all: build

build: cClient

cClient: cClient.o

tidy:
	rm -f *.jpg ./images/*.jpg

clean:
	rm -f cClient cClient.o

clobber: clean tidy
	rm -f ../bin/cClient

install:
	cp cClient ../bin
