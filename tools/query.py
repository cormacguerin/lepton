import sys
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
parser.add_argument("-e", "--endpoint",
                    help="search or suggest")

args = parser.parse_args()

filename = args.filename
endpoint = args.endpoint

if not endpoint:
    endpoint = 'search'

if (filename):
    with open(filename) as json_file:
        data = json.load(json_file)
else:
    data = json.load(sys.stdin)

print(data)

if data['table'] is None:
    print("Please provide a table name")
if data['database'] is None:
    print("Please provide a database")
if data['query'] is None:
    print("Please provide a query")

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

Host = "34.67.102.230"
KeyId = '3'
KeyName = 'demo'
ApiScope = endpoint
SecretKey = 'OWJRZWLY-YMPWZ3B4-YNFZYJFX-AWJ6EM13'
Method = 'GET'
Path = '/' + endpoint

# note params need to be sorted by code point
QueryParameters = urllib.parse.urlencode(data, quote_via=urllib.parse.quote)
print('data')
print(data)

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

url = 'https://' + Host + Path + '?' + QueryParameters
print(url)

FormattedHeaders = ([':'.join((k,v)) for k, v in Headers.items()])

c = pycurl.Curl()
c.setopt(c.URL,  url)
c.setopt(c.SSL_VERIFYPEER, 0)
c.setopt(c.SSL_VERIFYHOST, 0)
c.setopt(c.HTTPHEADER, FormattedHeaders)

c.perform()
c.close()
