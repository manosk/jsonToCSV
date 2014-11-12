CC = gcc
CPP = g++ 

CCOPT = -g -O3
CCFLAGS = -c $(CCOPT) 

#Assumes gtest has been built in the home directory of the user
USER_DIR = $(HOME)
CPPFLAGS = -I$(HOME)/include -I.

# Flags passed to the C++ compiler.
#CXXFLAGS += -g -pthread

RMOBJS = convertJSON convertJSON.o

all: convertJSON

convertJSON: convertJSON.cpp
	${CPP} ${CCOPT} ${CPPFLAGS} $(CXXFLAGS) $^ -o $@ ${LDFLAGS}
clean:
	rm -rf ${RMOBJS}

