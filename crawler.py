# -*- coding: utf-8 -*-
import base64
import datetime
import json
import pycurl
import re
import requests
import threading
import urllib
from io import BytesIO
from io import StringIO
from bs4 import BeautifulSoup
from bs4.element import Comment
from urlmatch import urlmatch
from multiprocessing.dummy import Pool as ThreadPool

#from urllib.parse import urlparse
try:
    from urllib.parse import urlparse
except ImportError:
     from urlparse import urlparse

headers = {'content-type': 'application/json'}
starturls = []
urlpatterns = []
geturls = set([])
goturls = set([])
num_threads = 10

def main():
    for line in open('patterns.urls', 'r').readlines():
        urlpatterns.append(line);
        
    for line in open('start.urls', 'r').readlines():
        line = line.split("#")[0]
        starturls.append(line.strip());
    crawl()


def crawl():
    if not geturls:
        urls = starturls.copy()
    else:
        urls = geturls.copy()

#  my way .. to do threading below but the threadpool seems nice too
#  so leaving that for now.
#
#    for t in range(num_threads):
#        if (urls):
#            runCrawl(urls.pop(0))

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

            # lets just remove now incase we get stuck in a loop
            if url in geturls:
                geturls.remove(url);

            # would be better to do this with curl maybe
            try:
                r = requests.post("http://127.0.0.1:3000/addDocument?type=content", data=data, headers=headers)
                print(r);
            except Exception:
                print('error unable to post to server.. server down?')
                return
            print("url " + r.url + " : " + r.text)



def urlMatch(url):
    for pattern in urlpatterns:
        if urlmatch(pattern, url):
            return True;
        else:
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

    if soup.find('title') is None:
        title = "";
        return ""
    else:
        title = soup.find('title').text

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
    data += title;
    data += text;

    try:
        lang = html['lang']
    except Exception:
        pass

    rawdata = base64.b64encode(data.encode())
    try:
        base64_string = rawdata.decode('UTF-8')
        #data = UrlData(body.decode('UTF-8'),head.decode('UTF-8'))
    except UnicodeEncodeError as e:
        print(str(e))
        return None

    urldata = {}
    urldata["display_url"] = url
    urldata["shell"] = ""
    urldata["tags"] = ""
    urldata["title"] = title
    urldata["metadata"] = ""
    urldata["last_modified"] = last_modified
    urldata["crawl_date"] = ""
    urldata["fetch_status"] = ""
    urldata["crawl_language"] = lang
    urldata["content_language"] = content_language
    urldata["content_type"] = content_type
    urldata["encoding"] = "base64"
    urldata["body"] = base64_string
    data = {url:urldata}
    json_data = json.dumps(data)
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
    try:
        c.perform()
    except pycurl.error:
        return None
    c.close()
    body = buffer.getvalue()
    head = headers.getvalue()
    try:
        data = UrlData(body.decode('UTF-8'),head.decode('UTF-8'))
    except UnicodeEncodeError as e:
        print(str(e))
        return None
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

main()

#def writeJson(url, data):
