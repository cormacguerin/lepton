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


 - testing.
 curl -H "Content-Type: application/json" -X POST --data "@testdocs.json" '127.0.0.1:3000/addDocument?type=content'
