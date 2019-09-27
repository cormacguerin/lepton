lepton is a indexing/serving framework.

Deps.

# BUILD SOFTWARE 
make
# there are two executables
# - indexroot (will read the documents from postgres and convert them in a reverse index in the index directory (it's pretty slow))
# - serveroot (a server that accepts queries (you can query from nodejs search endpoint))

 - python
 pip install pycurl requests bs4 urlmatch lxml --user

# DONT NEED SENTENCEPIECE - SKIP to rapidjson
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

 # USING RAPIDJSON NOW, INSTALL THIS
 - rapidjson (https://github.com/Tencent/rapidjson.git)
 git clone https://github.com/Tencent/rapidjson.git
 cd rapidjson
 cmake .
 make
 sudo make install


# install postgres and add the schema in the server directory
# install nodejs and run node lepton.js

 - testing indexing api.
 curl -H "Content-Type: application/json" -X POST --data "@testdocs.json" '127.0.0.1:3000/addDocument?type=content'
 - testing query api
curl 'https://35.239.29.200/search?query=test%20timing%202' -H 'accept: application/json' -H 'Connection: keep-alive' -H 'Accept-Encoding: gzip, deflate, br' -H 'Referer: https://35.239.29.200/' -H 'Accept-Language: en-US,en;q=0.9' -H 'User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/75.0.3770.142 Safari/537.36' --compressed --insecure -s -o /dev/null -w  "%{time_starttransfer}\n"


 - there is also a web crawler
 add your start urls (start.urls) and start url follow patters (patterns.urls)
 - to start the crawler type the below (ensure node lepton.js is running first!)
 python crawler.py

# THE FOLLOWING IS NOT NECESSARY - skip to DEBIAN INSTALL INSTRUCTION
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



# FULL INSTALL ON DEBIAN AS ROOT

apt-get install libicu57
apt-get install libicu-dev
apt-get install postgresql postgresql-server-dev-10 postgresql-server-dev-all postgresql-client


git clone https://github.com/Tencent/rapidjson.git
cd rapidjson/
cmake .
make
make install
cd ..


git clone https://github.com/jtv/libpqxx.git
cd libpqxx/
cmake -DPostgreSQL_TYPE_INCLUDE_DIR=/usr/include/postgresql/
./configure
make 
make install
cd ..


