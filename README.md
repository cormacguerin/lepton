lepton is a converged data solution with indexing/serving framework, machine learning and more.

# Howto setup on compute engine
# setup a new system and add nvme ssd in the disk management section for local ssd disk.
# log in and install theses.

sudo apt-get install postgresql
sudo apt-get install git
sudo apt-get install nodejs
sudo apt-get install xfsprogs
sudo apt-get install g++
sudo apt-get install build-essential
sudo apt-get install libpqxx-dev
sudo apt-get install libasio-dev
sudo apt-get install rapidjson-dev

# setup the ssd

# find the UUID
ls -l /dev/disk/by-uuid/

# add an entry like this replacing the correct UUID 
echo 'UUID=9c811e51-9e6e-441c-8508-80791411c468 /nvme xfs defaults 0 2' >> /etc/fstab

# add the mountpoint, filesystem and mount it.
sudo mkdir /nvme
sudo mkfs.xfs /dev/nvme0n1 
sudo mount /nvme

# make our directories and add the correct ownership and permissions
sudo mkdir /nvme/index
sudo chown compdeep:compdeep /nvme/index
sudo chmod 755 /nvme/index

sudo mkdir -p /nvme/postgresql/11/main/
sudo chown -R postgres:postgres /nvme/postgresql
sudo chmod -R 755 /nvme/postgresql

# pull the repo and cd into it
git clone https://github.com/cormacguerin/lepton.git
cd lepton

# create the index symlink
ln -s /nvme/index/ index

# BUILD SOFTWARE 
make
# there are two executables
# - indexroot (will read the documents from postgres and convert them in a reverse index in the index directory (it's pretty slow))
# - serveroot (a server that accepts queries (you can query from nodejs search endpoint))
# these are standalone applications that will run, I have yet to create a management script to start and stop them.

# install postgres create databases and add the schema in the server directory as postgres user
sudo su - postgres
createdb admin
createdb index 

# then run the schemas in server/ for each
cat /home/compdeep/lepton/server/admin_schema.psql |psql -d admin
cat /home/compdeep/lepton/server/index_schema.psql |psql -d index

# finally set a password for postgres user like this by logging in like this
psql
ALTER USER postgres PASSWORD '0fi1hakfpmaac1zmcx9nfa';
\q

# It will look something like this
# psql (11.7 (Debian 11.7-0+deb10u1))
# Type "help" for help.
#
# postgres=# ALTER USER postgres PASSWORD '0fi1hakfpmaac1zmcx9nfa';
# ALTER ROLE
# postgres=# \q

# all done here, exit back to compdeep account
exit

# you need to update the password in ./password with whatever is the psql postgres password
echo '0fi1hakfpmaac1zmcx9nfa' > .password

# we are now good to go just fire up nodejs
node lepton.js

# for the indexing to run when you have data all setup you need to manually then run index and serve in separeate screns
# for example
screen ./indexroot
screen ./serveroot

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


# web frontend
tere is a react frontend in web-app 
to build it enter the directory and execute..
npm install
npm run build

you can then restart nodejs and access at 127.0.0.1:3000
