import sys
import pycurl
import json
import urllib.parse
import hmac
import datetime
import pathlib

from io import BytesIO
from os import path
from argparse import ArgumentParser
from hashlib import sha256
from base64 import b64encode, b64decode

def hmacSha256(secret, msg):
    cmac = hmac.new(secret, msg.encode('utf-8'), sha256)
    return cmac.digest()

def genPayload():
    with open(sys.argv[1], "rb") as f:
        im_bytes = f.read()
    im_b64 = b64encode(im_bytes).decode("utf8")
    return {"image": im_b64, "action": "auto_correct"}

print(sys.argv[1])
data = genPayload()

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

Host = "intradeep.com"
KeyId = '3'
KeyName = 'colorcorrect'
ApiScope = 'model'
SecretKey = 'CTZNBJG4-C2LVMJRH-EWXJZWDQ-BGD5DWXS'
Method = 'POST'
Path = '/api/image'

# note params need to be sorted by code point
params = {}
QueryParameters = urllib.parse.urlencode(params, quote_via=urllib.parse.quote)

# credential is a / separated string of the key id, key name, datestamp and apiscope as created on the control panel
Credential = KeyId + '/' + Datestamp + '/' + ApiScope + '/lt_request'
# print(' - Credential ' + Credential)

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

    # print(' - SigningString ')
    # print(SigningString)
    # print(' - SigningKey hex')
    # print(genSigningKey().hex())

    Signature = hmac.new(genSigningKey(), SigningString.encode('utf-8'), sha256).hexdigest()
    # print(' - Signature ' + Signature)

    # construct authorization header and add into headers
    return 'LT-HMAC-SHA256 Credential=' + Credential + ',SignedHeaders=' + ';'.join(str(i) for i in HeadersToSign) + ',Signature=' + Signature

Headers['Authorization']  = genSignature()

# print(' - Headers')
# print(Headers)

base_url = 'https://' + Host + '/api/image?'
url = base_url + urllib.parse.urlencode(params, quote_via=urllib.parse.quote)

FormattedHeaders = ([':'.join((k,v)) for k, v in Headers.items()])

buffer = BytesIO()

c = pycurl.Curl()
c.setopt(c.URL,  url)
c.setopt(c.SSL_VERIFYPEER, 0)
c.setopt(c.SSL_VERIFYHOST, 0)
c.setopt(c.HTTPHEADER, FormattedHeaders)
c.setopt(c.POSTFIELDS, json.dumps(data))
c.setopt(c.WRITEDATA, buffer)

c.perform()
c.close()

# x = buffer.getvalue().decode('UTF-8')
response = buffer.getvalue()

head, tail = path.split(sys.argv[1])

try:
    response = json.loads(json.loads(response))
except Exception as err:
    print(err)

path = pathlib.Path("out")
path.mkdir(parents=True, exist_ok=True)
filepath = str(path) + '/' + str(tail)

img = b64decode(response["image"])
with open(filepath,"wb+") as f:
    f.write(img)

print("processed " + filepath)

