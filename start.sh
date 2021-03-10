#!/bin/bash

# /usr/lib/postgresql/13/bin/initdb pgdb/
# /usr/lib/postgresql/13/bin/postgres -D /home/invoke/pgdb -c config_file=/etc/postgresql/13/main/postgresql.conf&

# docker instance id
CID=$(basename $(cat /proc/1/cpuset))
if [ $CID = '/' ]; then
  echo "Failed to get docker instace ? $CID"
  exit 1
fi

# setup the database
# psql --command "CREATE USER invoke WITH SUPERUSER PASSWORD '$DB_PASSWORD';" &&\
# createdb -O invoke $CID

# echo "$( jq ".postgres_database = $CID" config.json )" > config.json

pm2 start bin/indexroot
pm2 start bin/serveroot

node invoke.js $(hostname -i)
status=$?
if [ $status -ne 0 ]; then
  echo "Failed to start my_first_process: $status"
  exit $status
fi
