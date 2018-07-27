
#
# Include Directories.
#
LOCALINC = /usr/local/include/
REDISINC = /usr/local/include/cpp_redis/
PQXXINC = /usr/include/pqxx/
PQXXINCLOCAL = /usr/local/include/pqxx/
RAPIDJSONINC = /usr/include/rapidjson/
INCLUDES = -I$(LOCALINC) -I$(REDISINC) -I$(PQXXINC) -I$(PQXXINCLOCAL) -I$(RAPIDJSONINC)
ICUOPTS = `/usr/bin/icu-config --ldflags --cppflags`

#
# Compiler and Linker Options.
#
COMPOPTS = -O0 -c -g -std=c++11 -std=c++1y -lpthread -lpqxx -lpq
LINKOPTS = -O0 -g -lstdc++ -lm -std=c++11 -std=c++1y -lpthread -lcpp_redis -lpqxx -lpq $(ICUOPTS)

#
# Library Files
#
REDIS_CLIENT_LIB = /usr/local/lib/libtacopie.a /usr/local/lib/libtacopie.a
SENTENCE_PIECE_LIB = /usr/local/lib/libsentencepiece.a  /usr/local/lib/libsentencepiece_train.a
PROTOBUFFER_LIB = /usr/lib64/libprotobuf.so

#
# Compiler
#
COMPILER = g++

#
# Linker Flags
#
#LD_FLAGS  = $(REDIS_CLIENT_LIB) $(SENTENCE_PIECE_LIB) $(PROTOBUFFER_LIB)
LD_FLAGS  = $(REDIS_CLIENT_LIB)
CFLAGS    = ${INCLUDES} 

#
# Specify our compiler and linker settings
#
COMPILE = $(COMPILER) $(COMPOPTS) $(INCLUDES) 
LINKER = $(COMPILER) $(LINKOPTS)

all: proton.o neutron.o base64.o segmenter.o
	${LINKER} -o atom proton.o neutron.o base64.o segmenter.o $(LD_FLAGS)

proton.o : proton.cc proton.h
	${COMPILE} proton.cc

neutron.o : neutron.cc
	${COMPILE} neutron.cc

segmenter.o : segmenter.cc segmenter.h
	${COMPILE} segmenter.cc

base64.o : base64.cc
	${COMPILE} base64.cc base64.h

#sentence_piece_processor.o : sentence_piece_processor.cc
#	${COMPILE} sentence_piece_processor.cc sentence_piece_processor.h

# clean
.PHONY: clean

clean:
	rm *.o atom
