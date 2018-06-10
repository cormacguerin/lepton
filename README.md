lepton is a indexing/serving framework.

Deps.

 - cpp_redis (https://github.com/google/sentencepiece)
 from https://github.com/Cylix/cpp_redis/wiki/Mac-&-Linux-Install

 # Clone the project
 git clone https://github.com/Cylix/cpp_redis.git
 # Go inside the project directory
 cd cpp_redis
 # Get tacopie submodule
 git submodule init && git submodule update
 # Create a build directory and move into it
 mkdir build && cd build
 # Generate the Makefile using CMake
 cmake .. -DCMAKE_BUILD_TYPE=Release
 # Build the library
 make
 # Install the library
 sudo make install


 - sentencepiece (https://github.com/google/sentencepiece)
 # install autotools if it's not already installed.
 apt-get install autotools
 # install google protobuf
 apt-get install protobuf-compiler
 # clone the sentencepiece 
 git clone https://github.com/google/sentencepiece.git
 ./autogen.sh
 make
 sudo make install

 - rapidjson (https://github.com/Tencent/rapidjson.git)
 git clone https://github.com/Tencent/rapidjson.git
 cd rapidjson
 cmake .
 make
 sudo make install


 - testing api.
 curl -H "Content-Type: application/json" -X POST --data "@testdocs.json" '127.0.0.1:3000/addDocument?type=content'


 - web crawler
 add your start urls (start.urls) and start url follow patters (patterns.urls)
 - to start the crawler type the below (ensure node lepton.js is running first!)
 python crawler.py

 - export the vocabulary.
 ./main

 - train the model on the exported vocab
 - english example (you need to build the unigram and word models for english, for CJK just unigram)
 spm_train --input=rawvocab.txt --model_type=unigram --model_prefix=unigram --model_type=unigram --vocab_size=10000
 spm_train --input=rawvocab.txt --model_type=word --model_prefix=unigram --model_type=word --vocab_size=10000
 - make an alphabetically sorted copy of these
 sort word.vocab > word.vocab.sorted
 sort unigram.vocab > unigram.vocab.sorted

 - build synonyms with electron.
 ./electron word.vocab.sorted


# linker stuff for icu
  g++  -o ngrams ngrams.cc  `/usr/bin/icu-config --ldflags --cppflags`
