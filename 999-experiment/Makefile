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

build: experiment

experiment: experiment.o

tidy:
	rm -f *.jpg

clean:
	rm -f experiment experiment.o

clobber: clean tidy
	rm -f ../bin/experiment

install:
	cp experiment ../bin
