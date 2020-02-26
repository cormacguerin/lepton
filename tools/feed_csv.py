import pycurl
import json
import urllib.parse

from argparse import ArgumentParser

parser = ArgumentParser()
parser.add_argument("-f", "--filename",
                    help="file name")
parser.add_argument("-d", "--database",
                    help="database name")
parser.add_argument("-t", "--table",
                    help="table name")

args = parser.parse_args()

if (args.table):
    table = args.table
else:
    table = ''
database = args.database
filename = args.filename

with open(filename) as json_file:
    data = json.load(json_file)

params = {'database':database,'table':table}
encoded_params = urllib.parse.urlencode(params)
print(encoded_params)

c = pycurl.Curl()
c.setopt(c.URL, 'https://35.239.29.200/addTableData?' + encoded_params)
c.setopt(c.SSL_VERIFYPEER, 0)
c.setopt(c.SSL_VERIFYHOST, 0)
c.setopt(c.HTTPHEADER, ['Content-Type: application/json','Accept-Charset: UTF-8'])
c.setopt(c.POSTFIELDS, json.dumps(data))
c.perform()
c.close()
