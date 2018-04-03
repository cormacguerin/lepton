import pycurl
import json
import base64
import requests
import urllib
from io import BytesIO
from bs4 import BeautifulSoup

headers = {'content-type': 'application/json'}
starturls = []
geturls = set([])
goturls = set([])

def main():
    for line in open('start.urls', 'r').readlines():
        line = line.split("#")[0]
        starturls.append(line.strip());
    crawl()


def crawl():
    if not geturls:
        urls = starturls.copy()
    else:
        urls = geturls.copy()

    for url in urls:
        url = sanitizeString(url);
        print('processing url ' + url)
        try:
            body = getUrl(url)
        except Exception:
            pass
        soup = BeautifulSoup(body, "lxml")
        data = buildPayload(url, soup)
        links = getLinks(url, soup)
        for link in links:
            # remove anchors
            link = link.split("#")[0]
            if not link in goturls:
                geturls.add(link)

        # lets just remove now incase we get stuck in a loop
        if url in geturls:
            geturls.remove(url);

        # would be better to do this with curl maybe
        r = requests.post("http://127.0.0.1:3000/addDocument?type=content", data=data, headers=headers)
        print("url " + r.url + " : " + r.text)

    crawl()


def buildPayload(url, soup):
    rawdata = base64.b64encode(soup.text.encode())
    base64_string = rawdata.decode('UTF-8')
    urldata = {}
    urldata['display_url'] = url
    urldata['shell'] = ''
    urldata['tags'] = ''
    urldata['title'] = ''
    urldata['metadata'] = ''
    urldata['encoding'] = 'base64'
    urldata['body'] = base64_string
    data = {url:urldata}
    json_data = json.dumps(data)
    return json_data

def getUrl(url):
    buffer = BytesIO()
    c = pycurl.Curl()
    c.setopt(c.URL, url)
    c.setopt(c.WRITEDATA, buffer)
    c.perform()
    c.close()
    body = buffer.getvalue()
    return body.decode('UTF-8')

def getLinks(url, soup):
    base_url = url.rsplit('/', 2)[0]
    hrefs = soup.find_all('a')
    links = []

    for tag in hrefs:
        link = tag.get('href',None)
        if link is not None:
            # again remove fragments
            link = link.split("#")[0]
            if link[0:4] == 'http':
                links.append(link);
            if link[:1] == '/':
                 links.append(base_url + link);
    return set(links)

def sanitizeString(s):
    s.replace("–","-")
    return s

main()

#def writeJson(url, data):
