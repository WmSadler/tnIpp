CFLAGS = `pkg-config --cflags opencv`
CFLAGS +=-ggdb

LDLIBS += -lzmq

all: build

build: cClient

testbed: cClient.o

run: build
	./cClient

tidy:
	rm -f *.jpg

clean: tidy
	rm -f cClient cClient.o

clobber: clean

install: 
