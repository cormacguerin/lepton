
#
# Include Directories.
#
INC = /usr/include/
LOCALINC = /usr/local/include/
#REDISINC = /usr/local/include/cpp_redis/
PQXXINC = /usr/include/pqxx/
ASIOINC = /usr/include/asio/
PQXXINCLOCAL = /usr/local/include/pqxx/
RAPIDJSONINC = /usr/include/rapidjson/
INCLUDES = -I$(INC) -I$(LOCALINC) -I$(REDISINC) -I$(PQXXINC) -I$(PQXXINCLOCAL) -I$(ASIOINC) -I$(RAPIDJSONINC)
ICUOPTS = `/usr/bin/icu-config --ldflags --cppflags`

#
# Compiler and Linker Options.
#
COMPOPTS = -O0 -c -g -std=c++11 -std=c++1y -lpthread -lpqxx -lpq
LINKOPTS = -O0 -g -lstdc++ -lm -std=c++11 -std=c++1y -lpthread -lpqxx -lpq $(ICUOPTS)

#
# Library Files
#
#REDIS_CLIENT_LIB = /usr/local/lib/libtacopie.a /usr/local/lib/libtacopie.a
SENTENCE_PIECE_LIB = /usr/local/lib/libsentencepiece.a  /usr/local/lib/libsentencepiece_train.a
PROTOBUFFER_LIB = /usr/lib64/libprotobuf.so

#
# Compiler
#
COMPILER = g++

#
# Linker Flags
#
# LD_FLAGS  = $(REDIS_CLIENT_LIB) $(SENTENCE_PIECE_LIB) $(PROTOBUFFER_LIB)
#LD_FLAGS  = $(REDIS_CLIENT_LIB)
CFLAGS    = ${INCLUDES} 

#
# Specify our compiler and linker settings
#
COMPILE = $(COMPILER) $(COMPOPTS) $(INCLUDES) 
LINKER = $(COMPILER) $(LINKOPTS)

all: serveroot indexroot

serveroot: serveroot.o query.o query_builder.o index_server.o server.o session.o segmenter.o
	${LINKER} -o serveroot serveroot.o query.o query_builder.o index_server.o server.o session.o segmenter.o $(LD_FLAGS)

indexroot: indexroot.o proton.o base64.o segmenter.o
	${LINKER} -o indexroot indexroot.o proton.o base64.o segmenter.o $(LD_FLAGS)

proton.o : proton.cc proton.h
	${COMPILE} proton.cc

query.o : query.cc query.h
	${COMPILE} query.cc

query_builder.o : query_builder.cc query_builder.h
	${COMPILE} query_builder.cc

index_server.o : index_server.cc index_server.h
	${COMPILE} index_server.cc

session.o : session.cc session.h
	${COMPILE} session.cc

server.o : server.cc server.h
	${COMPILE} server.cc

indexroot.o : indexroot.cc
	${COMPILE} indexroot.cc

serveroot.o : serveroot.cc
	${COMPILE} serveroot.cc

segmenter.o : segmenter.cc segmenter.h
	${COMPILE} segmenter.cc

base64.o : base64.cc
	${COMPILE} base64.cc base64.h

#sentence_piece_processor.o : sentence_piece_processor.cc
#	${COMPILE} sentence_piece_processor.cc sentence_piece_processor.h

# clean
.PHONY: clean

clean:
	rm *.o serveroot indexroot
