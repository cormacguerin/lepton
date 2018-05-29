
#
# Include Directories.
#
LOCALINC = /usr/local/include/
INCLUDES = -I$(LOCALINC)

#
# Compiler and Linker Options.
#
COMPOPTS = -O0 -c -g -std=c++11 -lpthread
LINKOPTS = -O0 -g -lstdc++ -lm -std=c++11 -lpthread

#
# Compiler
#
COMPILER = g++

#
# Linker Flags
#
LD_FLAGS  = $(REDIS_CLIENT_LIB) $(SENTENCE_PIECE_LIB) $(PROTOBUFFER_LIB)
CFLAGS    = ${INCLUDES} 

#
# Specify our compiler and linker settings
#
COMPILE = $(COMPILER) $(COMPOPTS) $(INCLUDES) 
LINKER = $(COMPILER) $(LINKOPTS)

all: quark.o neuron.o word2vec.o
	${LINKER} -o ml quark.o neuron.o word2vec.o $(LD_FLAGS)

quark.o : quark.cc quark.h
	${COMPILE} quark.cc

neuron.o : neuron.cc neuron.h
	${COMPILE} neuron.cc

word2vec.o : word2vec.cc word2vec.h
	${COMPILE} word2vec.cc

# clean
.PHONY: clean

clean:
	rm *.o ml
