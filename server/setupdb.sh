#/bin/bash

# this will be where we setup the postgres install
createdb -U postgres index --encoding=UTF8
psql -h localhost --username=postgres --list
createuser clio
psql -h localhost -U postgres -d index  -c "ALTER USER clio WITH PASSWORD 'secret';"
psql -h localhost -U postgres -d index  -c "\du;"
psql -h localhost -U postgres -d index  --file=schema.psql
