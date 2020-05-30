# lepton is a converged data solution with indexing/serving framework, machine learning and more.

# Howto setup on compute engine, below are instructions for manual setup
# We need to automate this and alse create relase builts etc and images we can just roll out.

# First setup a new system and add nvme ssd in the disk management section for local ssd disk.
# There are instructions here if for reference
# https://cloud.google.com/compute/docs/disks/local-ssd

# create additional locals (you may need more depending on languages/customers)
sudo locale-gen en_US.UTF-8 

# increase the system wide soft limit in /etc/security/limits.conf
*   soft    nofile  100000
# then run sysctl and log back in for the changes to take effect.
sysctl -p

# intall all software required.
sudo apt-get install postgresql
sudo apt-get install apache2
sudo apt-get install git
sudo apt-get install nodejs
sudo apt-get install xfsprogs
sudo apt-get install g++
sudo apt-get install build-essential
sudo apt-get install libpqxx-dev
sudo apt-get install libasio-dev
sudo apt-get install rapidjson-dev
sudo apt-get install screen
sudo apt-get install npm

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

# INSTALL NODE JS LIBS
npm install

# BUILD WEBSITE
# Install the website nodejs libs and build the website
cd vue-app
# Now edit src/main.js
# and change the Vue.prototype.$SERVER_URI
# Vue.prototype.$SERVER_URI = 'https://compdeep.customer.com'
npm install
npm run build

# done go back the lepton dir
cd ..

# BUILD SOFTWARE 
make
# there are two executables
# - indexroot (will read the documents from postgres and convert them in a reverse index in the index directory (it's pretty slow))
# - serveroot (a server that accepts queries (you can query from nodejs search endpoint))
# these are standalone applications that will run, I have yet to create a management script to start and stop them.
# for example
screen ./indexroot
screen ./serveroot

# SETUP DATABASE
sudo su - postgres

# configure the database directory.
/usr/lib/postgresql/11/bin/initdb -D /nvme/postgresql/11/main/

# create databases and add the schema in the server directory as postgres user
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
echo '0fi1hakfpmaac1zmcx9nfa' > dbpassword

# DONE
# we are now good to go just fire up nodejs
node lepton.js

# EXTRA FOR PRODICTION
# we are almost complete at this stage and we can actually run the app now but if we want to serve publically
# we need to route the node traffic, I'm using apache reverse proxy to do this in procutions, if you are running locally
# you can already skip and just run against 0:3000 in the browser
# 
# Apache setup follows for prod, jump into a root shell
sudo su -

a2enmod proxy
a2enmod proxy_http
a2enmod proxy_ajp
a2enmod rewrite
a2enmod deflate
a2enmod headers
a2enmod proxy_balancer
a2enmod proxy_connect
a2enmod proxy_html

# There is a reverse proxy config in /home/YOUR_USER_HOME_DIR/lepton/server/apache2.conf
# you can either copy this over in place of default or manually add the differences,
# Either way you will need to do some editing of the ip address to get it to work.

cp /home/compdeep/lepton/server/apache2.conf /etc/apache2/

# now we need to make certificates, for testing we will use self signed ones
# however if it were a real customer we would need to get them to provide the certs
# we can make self signed certs like this, I'm storing them in a cas folder so we need to
# create that first
mkdir /etc/apache2/cas/

# generate the keys and copy them, you will be asked some pass phrase, pick some string
# for real customers we will need to store this somewhere secure.
openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365
cp cert.pem /etc/apache2/cas/compdeep.pem 
cp cert.pem /etc/apache2/cas/compdeep.crt 
cp key.pem /etc/apache2/cas/compdeep.pem

# The edits that we need to make are as follows.
# Of course you need to change to ip to yours or whatever domainname
# eg. compdeep.example.com
#
# <VirtualHost *:80>
#         ServerName basic
#         Redirect permanent / https://34.67.102.230/
# </VirtualHost>
#
# <VirtualHost *:443>
# #    ServerName www.compdeep.com
#     ServerName 34.67.102.230
#     <Proxy *>
#         Order deny,allow
#         Allow from all
#     </Proxy>
#     SSLEngine on
#     SSLProtocol all -SSLv2
#     SSLCipherSuite HIGH:MEDIUM:!aNULL:!MD5
#     SSLProxyEngine On
#     SSLCertificateFile /etc/apache2/cas/compdeep.crt
#     SSLCertificateKeyFile /etc/apache2/cas/compdeep.pem
# #    SSLCertificateChainFile /etc/apache2/cas/DigiCertCA.crt
#     ProxyRequests Off
#     ProxyPreserveHost On
#     ProxyPass / http://127.0.0.1:3000/ Keepalive=On
#     ProxyPassReverse / http://127.0.0.1:3000/
# </VirtualHost>
# 

# 
# And that's it we can restart apache, there is more info on apache config here
# https://www.digitalocean.com/community/tutorials/how-to-use-apache-http-server-as-reverse-proxy-using-mod_proxy-extension
# Eventually we will need to setup more advances load balancing rules etc
#
# Finally restart apache , you will need to enter the password used creating the certs

service apache2 restart
# if something goes wrong take a look in the apache error logs in /var/log/apache2/error.log
# Otherwise we should be good to go, acceess the public IP or domain name
# you will get an insecure error of course if it's a self signed cert but should be good for testing.

# TESTING AND UsING THE API

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
