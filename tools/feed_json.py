import pycurl
import json
import urllib.parse
import hmac
import datetime

from os import path
from argparse import ArgumentParser
from hashlib import sha1
from base64 import b64encode

def hmacSha1(secret, string):
    string_to_sign = string.encode('utf-8')
    cmac = hmac.new(secret, string_to_sign, sha1)
    return cmac

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

KeyId = '8'
KeyName = 'cormacskey'
ApiScope = 'chart'
SecretKey = b'xyz'
Method = 'POST'
Host = '127.0.0.1'
Path = '/testApiKey'

# required headers (for signing) are date, content-tpye and host
HeadersToSign = ['content-type', 'host', 'date']
Headers = { 'Content-Type': 'application/json', 'Accept-Charset': 'UTF-8', 'Host': Host, 'Date': Timestamp }

# format headers into one lowercase string
def formatSigningHeaders():
    headerStr = ''
    for h in Headers.keys():
        if h.lower() in HeadersToSign:
            headerStr += h.lower()
            headerStr += ':'
            headerStr += Headers[h]
            headerStr += "\n"
    return headerStr
        
print(' - SigningHeaders ' + formatSigningHeaders())

# credential is a / separated string of the key id, key name, datestamp and apiscope as created on the control panel
Credential = KeyId + '/' + Datestamp + '/' + ApiScope + '/lt_request'
print(' - Credential ' + Credential)

# generate the signing key from the same data in the credential
def genSigningKey():
    keyDate = hmacSha1("LT" + SecretKey, Datestamp)
    KeyName = hmacSha1(keyDate, KeyName)
    keyScope = hmacSha1(keyName, ApiScope)
    keySigning = hmacSha1(keyService, 'lt_request')
    return keySigning

# Headers = 'content-type:application/json' + '\n' + 'accept-charset:utf-8' + '\n' + 'host:' + Host + '\n' + Date + '\n'

params = {'database':database,'table':table}
QueryParameters = urllib.parse.urlencode(params)

SigningString = Method + "\n" + Host + "\n" + Path + "\n" + QueryParameters + "\n" + formatSigningHeaders()

print(' - SigningString ')
print(SigningString)

SigningKey = hmacSha1(SecretKey, Credential).hexdigest()
print(' - SigningKey ' + SigningKey)
HMACSignature = hmacSha1(SigningKey.encode('utf-8'), SigningString).hexdigest()
print(' - HMACSignature ' + HMACSignature)
Signature = b64encode(HMACSignature.encode('utf-8')).decode('utf-8')
print(' - Signature ' + Signature)

AuthorizationHeader = 'Authorization: LT-HMAC-SHA256 Credential=' + Credential + ',SignedHeaders=' + ';'.join(str(i) for i in HeadersToSign) + ',Signature=' + Signature
print(AuthorizationHeader)

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
