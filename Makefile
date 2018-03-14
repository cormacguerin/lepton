
#
# Include Directories.
#
LOCALINC = /usr/local/include/
REDISINC = /usr/local/include/cpp_redis/
INCLUDES = -I$(LOCALINC) -I$(REDISINC)

#
# Compiler and Linker Options.
#
COMPOPTS = -O0 -c -g -std=c++11 -lpthread
LINKOPTS = -O0 -g -lstdc++ -lm -std=c++11 -lpthread -lcpp_redis

#
# Library Files
#
REDIS_CLIENT_LIB = /usr/local/lib/libtacopie.a /usr/local/lib/libtacopie.a

#
# Compiler
#
COMPILER = g++

#
# Linker Flags
#
LD_FLAGS  = $(REDIS_CLIENT_LIB)
CFLAGS    = ${INCLUDES} 

#
# Specify our compiler and linker settings
#
COMPILE = $(COMPILER) $(COMPOPTS) $(INCLUDES) 
LINKER = $(COMPILER) $(LINKOPTS)

all: proton.o neutron.o
	${LINKER} -o main proton.o neutron.o $(LD_FLAGS)

proton.o : proton.cc proton.h
	${COMPILE} proton.cc

neutron.o : neutron.cc
	${COMPILE} neutron.cc

# clean
.PHONY: clean

clean:
	rm *.o main
