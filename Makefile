
#
# Include Directories.
#
INC = /usr/include/
LOCALINC = /usr/local/include/
#REDISINC = /usr/local/include/cpp_redis/
PQXXINC = /usr/include/pqxx/
ASIOINC = /usr/include/asio/
PQXXINCLOCAL = /usr/local/include/pqxx/
PARALLEL_HASHMAP_INC = parallel-hashmap/
INCLUDES = -I$(INC) -I$(LOCALINC) -I$(REDISINC) -I$(PQXXINC) -I$(PQXXINCLOCAL) -I$(ASIOINC) -I$(PARALLEL_HASHMAP_INC)
#ICUOPTS = `/usr/bin/icu-config --ldflags --cppflags`

#
# Compiler and Linker Options.
#
COMPOPTS = -O0 -c -g -std=c++17 -lpthread -lpqxx -lpq -lstdc++fs
#LINKOPTS = -O0 -g -lstdc++ -lm -std=c++17 -lpthread -lpqxx -lpq $(ICUOPTS) -lstdc++fs
LINKOPTS = -O0 -g -lm -std=c++17 -lpthread -lpqxx -lpq $(ICUOPTS)
STATIC_LINKER = -lstdc++fs
#
# Library Files
#
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
LD_FLAGS  = $(shell pkg-config --libs icu-i18n RapidJSON) $(STATIC_LINKER)
CFLAGS    = ${INCLUDES} 

#
# Specify our compiler and linker settings
#
COMPILE = $(COMPILER) $(COMPOPTS) $(INCLUDES) 
LINKER = $(COMPILER) $(LINKOPTS)

all: serveroot indexroot

serveroot: serveroot.o query.o query_builder.o index_server.o server.o session.o segmenter.o frag_manager.o frag.o result.o
	${LINKER} -o serveroot serveroot.o query.o query_builder.o index_server.o server.o session.o segmenter.o frag_manager.o frag.o result.o $(LD_FLAGS)

indexroot: indexroot.o index_manager.o base64.o segmenter.o frag_manager.o frag.o murmur_hash3.o
	${LINKER} -o indexroot indexroot.o index_manager.o base64.o segmenter.o frag_manager.o frag.o murmur_hash3.o $(LD_FLAGS) 

index_manager.o : index_manager.cc index_manager.h
	${COMPILE} index_manager.cc

query.o : query.cc query.h
	${COMPILE} query.cc

result.o : result.cc result.h
	${COMPILE} result.cc

query_builder.o : query_builder.cc query_builder.h
	${COMPILE} query_builder.cc

index_server.o : index_server.cc index_server.h
	${COMPILE} index_server.cc

session.o : session.cc session.h
	${COMPILE} session.cc

server.o : server.cc server.h
	${COMPILE} server.cc

murmur_hash3.o : murmur_hash3.cc murmur_hash3.h
	${COMPILE} murmur_hash3.cc

frag.o : frag.cc frag.h
	${COMPILE} frag.cc

frag_manager.o : frag_manager.cc frag_manager.h
	${COMPILE} frag_manager.cc

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
