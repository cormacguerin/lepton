#/bin/bash
# remove previous entries
dropdb index
# this will be where we setup the postgres install
pw=$(openssl rand -base64 32)
psql -c "ALTER USER postgres WITH PASSWORD '$pw';"
echo "password " $pw
echo "password " $pw > password
export PGPASSWORD=$pw

createdb -U postgres index --encoding=UTF8
#psql -h localhost -U postgres -w -d index -c "\du;"
#psql -h localhost -U postgres -w --list
psql -h localhost -U postgres -w -d index --file=schema.psql
