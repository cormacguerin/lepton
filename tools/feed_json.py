import pycurl
import json
import urllib.parse
import hmac
import datetime

from os import path
from argparse import ArgumentParser
from hashlib import sha256
from base64 import b64encode

def hmacSha256(secret, msg):
    cmac = hmac.new(secret, msg.encode('utf-8'), sha256)
    return cmac.digest()

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

# UTC date
Date = datetime.datetime.utcnow()
Timestamp = Date.strftime('%Y%m%dT%H%M%SZ')
Datestamp = Date.strftime('%Y%m%d')

# The following values are used in the request authorization signing process
# Please update as necessary (ie create an api key on the frontend and copy
# the key, value, scope below) There is also a /testApiKey Path endpoing you can
# use for debugging api key issues.
#
# In addition to key and scope information you also need to provide the 
# Request Method, Host and Path. Path is the api endpoint including leading forward slash

KeyId = '2'
KeyName = 'wikipieda'
ApiScope = 'data'
SecretKey = 'ZDH3A2H4-EJLMAMLZ-ZZG5Y243-A3G2CJN3'
Method = 'POST'
Host = 'cormac.io'
Path = '/addTableData'

# note params need to be sorted by code point
params = {'database':database,'table':table}
QueryParameters = urllib.parse.urlencode(params)

# credential is a / separated string of the key id, key name, datestamp and apiscope as created on the control panel
Credential = KeyId + '/' + Datestamp + '/' + ApiScope + '/lt_request'
print(' - Credential ' + Credential)

# required headers (for signing) are date, content-tpye and host (note headers need to be sorted by code point)
HeadersToSign = ['content-type', 'date', 'host']
Headers = { 'Content-Type': 'application/json', 'Accept-Charset': 'UTF-8', 'Date': Timestamp, 'Host': Host }

# format headers into one lowercase string (make sure everything is lowercase)
def formatSigningHeaders():
    headerStr = ''
    for h in Headers.keys(): 
        if h.lower() in HeadersToSign:
            headerStr += h.lower()
            headerStr += ':'
            headerStr += Headers[h].lower()
            headerStr += "\n"
    return headerStr

# generate the signing key from the same data in the credential
def genSigningKey():
    keyDate = hmacSha256(bytes("LT" + SecretKey,'utf-8'), Datestamp)
    keyName = hmacSha256(keyDate, KeyName)
    keyScope = hmacSha256(keyName, ApiScope)
    keySigning = hmacSha256(keyScope, 'lt_request')
    return keySigning

def genSignature():

    SigningString = Method + "\n" + Host + "\n" + Path + "\n" + QueryParameters + "\n" + formatSigningHeaders()

    print(' - SigningString ')
    print(SigningString)
    print(' - SigningKey hex')
    print(genSigningKey().hex())

    Signature = hmac.new(genSigningKey(), SigningString.encode('utf-8'), sha256).hexdigest()
    print(' - Signature ' + Signature)

    # construct authorization header and add into headers
    return 'LT-HMAC-SHA256 Credential=' + Credential + ',SignedHeaders=' + ';'.join(str(i) for i in HeadersToSign) + ',Signature=' + Signature

Headers['Authorization']  = genSignature()

print(' - Headers')
print(Headers)

base_url = 'https://35.239.29.200/addTableData?'
url = base_url + urllib.parse.urlencode(params)

FormattedHeaders = ([':'.join((k,v)) for k, v in Headers.items()])

c = pycurl.Curl()
c.setopt(c.URL, 'https://35.239.29.200/addTableData?' + QueryParameters)
c.setopt(c.URL,  url)
c.setopt(c.SSL_VERIFYPEER, 0)
c.setopt(c.SSL_VERIFYHOST, 0)
c.setopt(c.HTTPHEADER, FormattedHeaders)
c.setopt(c.POSTFIELDS, json.dumps(data))

c.perform()
c.close()
