#
# example Dockerfile for https://docs.docker.com/engine/examples/postgresql_service/
#

FROM ubuntu:20.10

ARG DEBIAN_FRONTEND=noninteractive
ARG db_password=invoke

ENV POSTGRES_DB lepton
ENV DB_PASSWORD=$db_password
ENV LC_ALL en_US.UTF-8
ENV LANG en_US.UTF-8
ENV LANGUAGE en_US.UTF-8
# ENV TZ="Japan/Tokyo"

RUN apt-get update && apt-get install -y curl gnupg locales locales-all iproute2 ssl-cert postgresql-server-dev-all libicu-dev git rapidjson-dev cmake libpq-dev python3 g++ vim wget jq libasio-dev libpqxx-dev

RUN locale-gen en_US.UTF-8 
RUN update-locale

RUN echo 'LANGUAGE="en_US.UTF-8"' >> /etc/default/locale

RUN mkdir ~/.gnupg && echo "disable-ipv6" >> ~/.gnupg/dirmngr.conf

RUN curl -sL https://deb.nodesource.com/setup_14.x  | bash -
RUN apt-get install -y software-properties-common nodejs build-essential

RUN useradd -G users,ssl-cert invoke -d /home/invoke
RUN mkdir /home/invoke /home/invoke/postgres /home/invoke/src /home/invoke/vue-app /home/invoke/build /home/invoke/bin /home/invoke/src/parallel_hashmap /home/invoke/.npm 
RUN chown -R invoke /home/invoke

# apt doesnt have the right versions available so build from source
# RUN git clone https://github.com/jtv/libpqxx.git
# RUN cd libpqxx && git checkout 6.2.5 && ./configure --disable-documentation && make -j 2 && make install
# RUN cd

RUN git clone https://github.com/Tencent/rapidjson.git
RUN cd rapidjson && cmake . && make -j 2 && make install
RUN cd

# RUN wget https://launchpad.net/ubuntu/+archive/primary/+sourcefiles/asio/1:1.12.2-1/asio_1.12.2.orig.tar.gz && tar -xvzf asio_1.12.2.orig.tar.gz
# RUN cd asio-1.12.2/ && ./configure && make -j 2 && make install
# RUN cd

# install pm2 globally
RUN npm install pm2@latest -g

# COPY ./asio-1.18.1/include/ /usr/include/

# Expose the node port
EXPOSE 3000

# setup app
USER invoke
WORKDIR /home/invoke

COPY ./package.json /home/invoke/
COPY ./*.js /home/invoke/
COPY ./start.sh /home/invoke/
COPY ./config.json /home/invoke/
COPY ./src/* /home/invoke/src/
COPY ./vue-app/* /home/invoke/vue-app/
COPY ./src/parallel_hashmap/* /home/invoke/src/parallel_hashmap/
COPY ./Makefile /home/invoke/

# make the index directory
RUN mkdir -p "/home/invoke/index/$(cat config.json |jq -r '.postgres_database')/docs/"

# build
RUN g++ --version
RUN make

# install nodejs packages
RUN npm install 
RUN npm install bcrypto

# install and build web app
# RUN npm install --prefix vue-app/
# RUN npm run build --prefix vue-app/

CMD ./start.sh

# Note: The official Debian and Ubuntu images automatically ``apt-get clean``
# after each ``apt-get``

