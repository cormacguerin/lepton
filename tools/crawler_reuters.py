# -*- coding: utf-8 -*-
import base64
import json
import pycurl
import re
# import requests
import threading
import urllib
from io import BytesIO
from io import StringIO
from bs4 import BeautifulSoup
from bs4.element import Comment
from multiprocessing.dummy import Pool as ThreadPool
import hmac
from datetime import datetime
from os import path
from argparse import ArgumentParser
from hashlib import sha256
from base64 import b64encode
try:
    from urllib.parse import urlparse
except ImportError:
     from urlparse import urlparse


def hmacSha256(secret, msg):
    cmac = hmac.new(secret, msg.encode('utf-8'), sha256)
    return cmac.digest()

parser = ArgumentParser()
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

Params = {'database':database,'table':table}

starturls = []
urlpatterns = []
geturls = set([])
goturls = set([])
num_threads = 4

# The following values are used in the request authorization signing process
# Please update as necessary (ie create an api key on the frontend and copy
# the key, value, scope below) There is also a /testApiKey Path endpoing you can
# use for debugging api key issues.
#
# In addition to key and scope information you also need to provide the 
# Request Method, Host and Path. Path is the api endpoint including leading forward slash

KeyId = '2'
KeyName = 'demo'
ApiScope = 'data'
SecretKey = 'BJYYDW94-ZMC3YWI1-NWDSNDG1-CTE3YWPJ'
Method = 'POST'
Host = 'cormac.io'
Path = '/addTableData'
HeadersToSign = ['content-type', 'date', 'host']

def encodeParams():
    # note params need to be sorted by code point
    QueryParameters = urllib.parse.urlencode(Params, quote_via=urllib.parse.quote)
    return QueryParameters

# format headers into one lowercase string (make sure everything is lowercase)
def formatSigningHeaders(Headers):
    headerStr = ''
    for h in Headers.keys(): 
        if h.lower() in HeadersToSign:
            headerStr += h.lower()
            headerStr += ':'
            headerStr += Headers[h].lower()
            headerStr += "\n"
    return headerStr

# generate the signing key from the same data in the credential
def genSigningKey(Datestamp):
    keyDate = hmacSha256(bytes("LT" + SecretKey,'utf-8'), Datestamp)
    keyName = hmacSha256(keyDate, KeyName)
    keyScope = hmacSha256(keyName, ApiScope)
    keySigning = hmacSha256(keyScope, 'lt_request')
    return keySigning

def genSignature(Headers, Datestamp):

    SigningString = Method + "\n" + Host + "\n" + Path + "\n" + encodeParams() + "\n" + formatSigningHeaders(Headers)

    # print(' - SigningString ')
    # print(SigningString)
    # print(' - SigningKey hex')
    # print(genSigningKey(Datestamp).hex())

    Signature = hmac.new(genSigningKey(Datestamp), SigningString.encode('utf-8'), sha256).hexdigest()
    # print(' - Signature ' + Signature)

    # credential is a / separated string of the key id, key name, datestamp and apiscope as created on the control panel
    Credential = KeyId + '/' + Datestamp + '/' + ApiScope + '/lt_request'
    # print(' - Credential ' + Credential)

    # construct authorization header and add into headers
    return 'LT-HMAC-SHA256 Credential=' + Credential + ',SignedHeaders=' + ';'.join(str(i) for i in HeadersToSign) + ',Signature=' + Signature

def crawl():
    if not geturls:
        urls = starturls.copy()
    else:
        urls = geturls.copy()

#  my way .. to do threading below but the threadpool seems nice too
#  so leaving that for now.
#
#   for t in range(num_threads):
#       if (urls):
#           runCrawl(urls.pop(0))

    pool = ThreadPool(num_threads) 
    results = pool.map(runCrawl, urls)
    pool.close();
    pool.join();
    print(results);

    crawl()


def runCrawl(url):
        url = sanitizeString(url);
        print('processing url ' + url)
        try:
            urldata = getUrl(url)
        except Exception:
            # we should set this to an error code and then mark for no recrawl
            geturls.remove(url);
            return
        if urldata:
            body = urldata.body
            head = urldata.head
            soup = BeautifulSoup(body, "lxml")
            data = buildPayload(url, soup, head)
            links = getLinks(url, soup)
            for link in links:
                # remove anchors
                link = link.split("#")[0]
                if (not link in goturls) and (len(geturls) < 10000):
                    geturls.add(link)

            print(data)

            # lets just remove now incase we get stuck in a loop
            if url in geturls:
                geturls.remove(url);

            # UTC date
            Date = datetime.utcnow()
            Timestamp = Date.strftime('%Y%m%dT%H%M%SZ')
            Datestamp = Date.strftime('%Y%m%d')

            # required headers (for signing) are date-tpye and host (note headers need to be sorted by code point)
            Headers = { 'Content-Type': 'application/json', 'Accept-Charset': 'UTF-8', 'Date': Timestamp, 'Host': Host }

            Headers['Authorization']  = genSignature(Headers, Datestamp)

            # print(' - Headers')
            # print(Headers)

            base_url = 'https://34.67.102.230/addTableData?'
            post_url = base_url + urllib.parse.urlencode(Params, quote_via=urllib.parse.quote)

            FormattedHeaders = ([':'.join((k,v)) for k, v in Headers.items()])
            # headers = {'content-type': 'application/json'}
            # print(post_url)
            # print(data)

            c = pycurl.Curl()
            c.setopt(c.URL, post_url)
            c.setopt(c.FOLLOWLOCATION, 1)
            c.setopt(c.SSL_VERIFYPEER, 0)
            c.setopt(c.SSL_VERIFYHOST, 0)
            c.setopt(c.HTTPHEADER, FormattedHeaders)
            # c.setopt(c.POSTFIELDS, json.dumps(data))
            if data:
                print(data)
            else:
                return
            c.setopt(c.POSTFIELDS, data)

            try:
                c.perform()
                # r = requests.post(post_url, data=data, headers=FormattedHeaders, verify=false)
            except Exception:
                print('error unable to post to server.. server down?')
                return
            c.close()
            # quit()


def urlMatch(url):
    for pattern in urlpatterns:
        if re.match(r"%s" % pattern, url):
            return True;
    return False;


def getDomain(url):
    parsed_uri = urlparse(url)
    domain = '{uri.scheme}://{uri.netloc}'.format(uri=parsed_uri)
    return domain


def tag_visible(element):
    if element.parent.name in ['style', 'script', 'head', 'title', 'meta', '[document]']:
        return False
    if isinstance(element, Comment):
        return False
    return True


def text_from_html(soup):
#    soup = BeautifulSoup(body, 'html.parser')
    texts = soup.findAll(text=True)
    visible_texts = filter(tag_visible, texts)  
    return u" ".join(t.strip() for t in visible_texts)


def buildPayload(url, soup, head):
    text = text_from_html(soup)
    html = soup.find('html')

    if soup.find("meta",  property="og:title") is None:
        title = ""
    else:
        title = soup.find("meta",  property="og:title")

    if soup.find("meta",  property="og:url") is None:
        url = ""
    else:
        url = soup.find("meta",  property="og:url")

    if soup.find("meta",  property="og:type") is None:
        type = ""
    else:
        type = soup.find("meta",  property="og:type")

    if soup.find("meta", property="og:article:modified_time") is None:
        published_date = ""
    else:
        published_date = soup.find("meta", property="og:article:modified_time")

    if soup.find("meta", property="og:locale") is None:
        return
    else:
        locale = soup.find("meta", property="og:locale")

    if locale:
        if locale["content"]:
            language = locale["content"][0:2]
        else:
            return

    metadata = {}
    metadata["source"] = "reuters"
    if type:
        metadata["type"] = type["content"]
    if published_date:
        metadata["published_date"] = datetime.fromisoformat(published_date["content"][0:10]).timestamp()*1000

    divs = soup.find_all("div", class_="reflist")
    for d in divs:
        d.decompose()

    last_modified = ''
    content_language = ''
    content_type = ''

    for line in head.splitlines():
        if 'odified' in line: 
          last_modified = re.sub('Last-Modified:', '', line.strip())
        if 'anguage' in line: 
          content_language = re.sub('Content-language:', '', line.strip())
        if 'Content-Type' in line: 
          content_type = re.sub('Content-Type:', '', line.strip())

    lang = ''
    data = "";
    if title:
        data += title["content"];
    data += text;

    try:
        lang = html['lang']
    except Exception:
        pass

    rawdata = base64.b64encode(data.encode())
    try:
        base64_string = rawdata.decode('UTF-8')
        # data = UrlData(body.decode('UTF-8'),head.decode('UTF-8'))
    except UnicodeEncodeError as e:
        print(str(e))
        return None

    urldata = {}
    if url:
       urldata["url"] = url["content"]
    else:
        return None
    # urldata["shell"] = ""
    # urldata["tags"] = ""
    if title:
        urldata["title"] = title["content"]
    else:
        return None




    urldata["last_modified"] = last_modified.strip()
    urldata["language"] = language
    # urldata["crawl_date"] = ""
    # urldata["fetch_status"] = ""
    # urldata["crawl_language"] = lang
    # urldata["language"] = content_language.strip()
    # urldata["content_type"] = content_type
    # urldata["encoding"] = "base64"
    # urldata["body"] = base64_string
    urldata["document"] = data.strip()
    urldata["metadata"] = json.dumps(metadata)
    # data = {url:urldata}
    json_data = json.dumps(urldata)
    return json_data


def getUrl(url):
    buffer = BytesIO()
    headers = BytesIO()
    c = pycurl.Curl()
    c.setopt(c.URL, url)
    c.setopt(c.WRITEDATA, buffer)
    c.setopt(c.CONNECTTIMEOUT, 10)
    # https://stackoverflow.com/questions/9191668/error-longjmp-causes-uninitialized-stack-frame
    c.setopt(c.NOSIGNAL, 1)
    c.setopt(c.HTTPHEADER, [
        'User-agent: KumaCrawl',
        'accept:text/html,application/xhtml+xml,application/xml;',
        'cache-control:max-age=0',
        ])
    c.setopt(c.HEADERFUNCTION, headers.write)
    print('deb 0')
    try:
        print('deb a')
        c.perform()
    except pycurl.error:
        print('deb b')
        return None
    c.close()
    body = buffer.getvalue()
    head = headers.getvalue()
    try:
        print('deb c')
        data = UrlData(body.decode('UTF-8'),head.decode('UTF-8'))
    except UnicodeEncodeError as e:
        print('deb d')
        print(str(e))
        return None
    print('deb e')
    return data


def getLinks(url, soup):
    domain = getDomain(url)
    base_url = url.rsplit('/', 2)[0]
    hrefs = soup.find_all('a')
    links = []

    for tag in hrefs:
        link = tag.get('href',None)
        if link is not None:
            # again remove fragments
            link = link.split("#")[0]
            if link[:1] == '/':
                 links.append(domain + link);
            if urlMatch(link):
                links.append(link);
    return set(links)

def sanitizeString(s):
    s.replace("–","-")
    s.replace("–","-")
    s.replace("—","-")
    s.replace('\u2013',"-")
    return s

class UrlData(object):
    body = ""
    head = ""

    # constructor
    def __init__(self, body, head):
        self.body = body
        self.head = head

def main():
    for line in open('patterns.urls', 'r').readlines():
        urlpatterns.append(line);
        
    for line in open('start.urls', 'r').readlines():
        line = line.split("#")[0]
        starturls.append(line.strip());
    crawl()

main()
#def writeJson(url, data):
