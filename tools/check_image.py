import sys
import json
import pycurl
import urllib.parse
import hmac
import datetime

from os import path
from argparse import ArgumentParser
from hashlib import sha256
from urllib.parse import urlparse
from base64 import b64encode

def download(url,filename):

    crl = pycurl.Curl()
    crl.setopt(crl.URL, url)

    with open(filename, 'wb') as f:
        crl.setopt(crl.WRITEFUNCTION, f.write)
        crl.perform()

    print(crl.getinfo(pycurl.HTTP_CODE))
    status = crl.getinfo(pycurl.HTTP_CODE)
    print(status)

    # End curl session
    crl.close()

    # Decode the bytes stored in get_body to HTML and print the result
    return status

filename = 'flipkart.json'

if (filename):
    with open(filename) as json_file:
        data = json.load(json_file)
else:
    data = json.load(sys.stdin)

newjson = []

for i in data:
    images = []
    for j in i['image']:
        found = False
        uri = urlparse(j)
        filename = 'assets/' + path.basename(uri.path)
        if (download(j,filename) == 200):
            images.append('https://compdeep.com/img/demo/' +filename)
            found = True
    if found:
        i['image'] = images
        print(i)
        newjson.append(i)

with open('working_flipkart.json', 'w') as outfile:
    json.dump(newjson, outfile)


