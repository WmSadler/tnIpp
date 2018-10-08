CXXFLAGS = `pkg-config --cflags opencv4`
CXXFLAGS += `pkg-config --cflags libzmq`
CXXFLAGS +=-std=c++11 -Ofast -march=native
CXXFLAGS +=-ggdb

LDLIBS = `pkg-config --libs opencv4`
LDLIBS = `pkg-config --libs libzmq`
LDLIBS += `pkg-config --libs blas`
LDLIBS += `pkg-config --libs lapack`
LDLIBS += -lstdc++ -lzmq

all: tryStuff
