

#  &copy; Cormac Guerin , Invoke Network
#  Copyright 2021 All Rights Reserved.
#
#  All information contained herein is, and remains
#  the property of Cormac Guerin & Invoke Network
#
#  The intellectual and technical concepts within remain
#  the sole property of the aforementioned owners.
#
#  Reproduction and / or distriptions of this software is 
#  strictly prohibited.



#
# Include Directories.
#
INC = /usr/include/
LOCALINC = /usr/local/include/
PQXXINC = /usr/include/pqxx/
ASIOINC = /usr/include/asio/
# PQXXINCLOCAL = /usr/local/include/pqxx/
PARALLEL_HASHMAP_INC = parallel-hashmap/
INCLUDES = -I$(INC) -I$(LOCALINC) -I$(REDISINC) -I$(PQXXINC) -I$(ASIOINC) -I$(PARALLEL_HASHMAP_INC)
#ICUOPTS = `/usr/bin/icu-config --ldflags --cppflags`

#
# Compiler and Linker Options.
#
COMPOPTS = -O0 -c -g -std=c++17 -pthread -lpqxx -lpq -lstdc++fs -o build/$@
LINKOPTS = -O0 -g -lm -std=c++17 -pthread -lpqxx -lpq $(ICUOPTS)
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

all: bin/serveroot bin/indexroot

bin/serveroot: serveroot.o base64.o query.o query_builder.o index_server.o management_server.o query_server.o session.o segmenter.o frag_manager.o frag.o result.o pg_pool.o
	${COMPILER} -o bin/serveroot build/serveroot.o build/base64.o build/query.o build/query_builder.o build/index_server.o build/management_server.o build/query_server.o build/session.o build/segmenter.o build/frag_manager.o build/frag.o build/result.o build/pg_pool.o $(LD_FLAGS) $(LINKOPTS)

bin/indexroot: indexroot.o index_manager.o base64.o segmenter.o frag_manager.o frag.o murmur_hash3.o
	${COMPILER} -o bin/indexroot build/indexroot.o build/index_manager.o build/base64.o build/segmenter.o build/frag_manager.o build/frag.o build/murmur_hash3.o $(LD_FLAGS) $(LINKOPTS)

index_manager.o : src/index_manager.cc src/index_manager.h
	${COMPILE} src/index_manager.cc

#score_document.o : score_document.cc score_document.h
#	${COMPILE} score_document.cc

query.o : src/query.cc src/query.h
	${COMPILE} src/query.cc

result.o : src/result.cc src/result.h
	${COMPILE} src/result.cc

query_builder.o : src/query_builder.cc src/query_builder.h
	${COMPILE} src/query_builder.cc

index_server.o : src/index_server.cc src/index_server.h
	${COMPILE} src/index_server.cc

session.o : src/session.cc src/session.h
	${COMPILE} src/session.cc

management_server.o : src/management_server.cc src/management_server.h
	${COMPILE} src/management_server.cc

query_server.o : src/query_server.cc src/query_server.h
	${COMPILE} src/query_server.cc

murmur_hash3.o : src/murmur_hash3.cc src/murmur_hash3.h
	${COMPILE} src/murmur_hash3.cc

frag.o : src/frag.cc src/frag.h
	${COMPILE} src/frag.cc

frag_manager.o : src/frag_manager.cc src/frag_manager.h
	${COMPILE} src/frag_manager.cc

indexroot.o : src/indexroot.cc
	${COMPILE} src/indexroot.cc

serveroot.o : src/serveroot.cc
	${COMPILE} src/serveroot.cc

segmenter.o : src/segmenter.cc src/segmenter.h
	${COMPILE} src/segmenter.cc

base64.o : src/base64.cc src/base64.h
	${COMPILE} src/base64.cc

pg_pool.o : src/pg_pool.cc src/pg_pool.h
	${COMPILE} src/pg_pool.cc

#sentence_piece_processor.o : sentence_piece_processor.cc
#	${COMPILE} sentence_piece_processor.cc sentence_piece_processor.h

# clean
.PHONY: clean

clean:
	rm build/*.o bin/serveroot bin/indexroot
