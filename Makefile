
#
# Include Directories.
#
LOCALINC = /usr/local/include/
REDISINC = /usr/local/include/cpp_redis/
INCLUDES = -I$(LOCALINC) -I$(REDISINC)

#
# Compiler and Linker Options.
#
COMPOPTS = -O0 -c -g -std=c++11
LINKOPTS = -O0 -g -lstdc++ -lm -std=c++11

#
# Library Files
#
REDIS_CLIENT_LIB = /usr/local/lib/libcpp_redis.a

#
# Compiler
#
COMPILER = g++

#
# Specify our compiler and linker settings
# For Mac you need to add OBJCOPTS (this is to get cocoa to jam with sdl2)
#
COMPILE = $(COMPILER) $(COMPOPTS) $(INCLUDES)
LINKER = $(COMPILER) $(LINKOPTS) $(REDIS_CLIENT_LIB)

all: proton.o
	${LINKER} -o main proton.o

proton.o : proton.cc
	${COMPILE} proton.cc

# clean
.PHONY: clean

clean:
	rm *.o main
