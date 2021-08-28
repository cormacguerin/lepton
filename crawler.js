var supercrawler = require("supercrawler");
var psl = require('psl');
// var urlMod = require("url");
var db_pg = {}

const textract = require('textract')

const pg = require('./api/postgres.js');

const fs = require('fs')

const buffer = require('buffer')

const getMetaData = require('metadata-scraper')

class Crawler {

  constructor(db_,tb_) {

    this.database = db_
    this.table = tb_
    this.status_ = "disabled"

    var init = async function(database, callback) {

      await loadConfig(function(r) {
        console.log('config.json loaded')
      })

      var db_config = {}
      db_config['postgres_database'] = db_
      db_config['postgres_password'] = config.postgres_password
      db_config['postgres_host'] = config.postgres_host
      db_config['postgres_user'] = config.postgres_user
      db_pg = new pg(db_config)

      return new supercrawler.Crawler({
        // By default, Supercrawler uses a simple FIFO queue, which doesn't support
        // retries or memory of crawl state. For any non-trivial crawl, you should
        // create a database. Provide your database config to the constructor of
        // DbUrlList.
        urlList: new supercrawler.DbUrlList({
          db: {
            database: db_,
            username: db_config.postgres_user,
            password: db_config.postgres_password,
            sequelizeOpts: {
              dialect: "postgres",
              host: db_config.postgres_host
            }
          }
        }),
        // Tme (ms) between requests
        interval: 300,
        // Maximum number of requests at any one time.
        concurrentRequestsLimit: 5,
        // Time (ms) to cache the results of robots.txt queries.
        robotsCacheTime: 3600000,
        // Query string to use during the crawl.
        userAgent: "Mozilla/5.0 (compatible; supercrawler/1.0; +https://github.com/brendonboshell/supercrawler)",
        // Custom options to be passed to request.
        request: {
          headers: {
            'x-custom-header': 'example'
          }
        }
      });
    }

    init().then(crawler=> {

      let vm = this
      db_pg.getCrawlerUrls(function(e,r) {
        var domains=[]
        for (var i=0; i < r.length; i++) {
          let d = psl.parse(extractHostname(r[i].url))
          if (d.subdomain && d.domain) {
            domains.push(d.subdomain + "." + d.domain)
          } else if (d.domain) {
            domains.push(d.domain)
          }
        }
        vm.domains = domains
        console.log(domains)

        crawler.addHandler(
          ["application/gzip",
           "image/gif",
           "image/bmp",
           "image/jpeg",
           "image/jpg",
           "image/png",
           "image/vnd.microsoft.icon",
           "image/tiff",
           "image/webp",
           "image/svg+xml",
           "audio/mpeg",
           "audio/ogg",
           "audio/wav",
           "audio/webm",
           "video/x-msvideo",
           "video/mp4",
           "video/ogg",
           "video/mpeg",
           "video/3gpp",
           "video/3gpp2",
           "video/mp2t",
           "video/webm",
           "video/mp2t",
           "application/java-archive",
           "application/json",
           "application/ld+json",
           "application/x-tar",
           "application/xhtml+xml",
           "application/zip",
           "application/x-httpd-php",
           "application/x-7z-compressed",
           "font/ttf",
           "font/woff",
           "font/woff2",
           "font/otf",
           "text/javascript",
           "text/csv"
          ], supercrawler.handlers.htmlLinkParser({
            hostnames: domains,         
            urlFilter: function(url) {
              return false
            }
        }));

        crawler.addHandler("text/html", supercrawler.handlers.htmlLinkParser({
          // Restrict discovered links to the following hostnames.
          hostnames: domains,         
          urlFilter: function(url) {
            if (url == undefined) {
              return false
            }
            const ext_re = /(http)?s?:?(\/\/[^"']*\.(?:png|jpg|jpeg|gif|png|svg|zip|gz|tar|rar|mp4|mov|avi)|.*%3A%2F%2F.*)/g
            if (url.match(ext_re)) {
              return false
            }
            var domain
            let d = psl.parse(extractHostname(url))
            if (d.subdomain && d.domain) {
              domain = d.subdomain + "." + d.domain
            } else if (d.domain) {
              domain = d.domain
            }
            if (domains.indexOf(domain) === -1) {
              console.log("urlFilter " + domain + " rejected")
              return false
            }
            console.log("urlFilter " + domain + " accepted")
            return true
          }
        }));

        // Get "Sitemaps:" directives from robots.txt
        crawler.addHandler(supercrawler.handlers.robotsParser());

        // Crawl sitemap files and extract their URLs.
        crawler.addHandler(supercrawler.handlers.sitemapsParser());
        
        crawler.addHandler(["text/plain", "text/html"], parseHandler.bind(this,domains));

        for (var i=0; i < r.length; i++) {
          crawler.getUrlList()
          //.insertIfNotExists(new supercrawler.Url(r[i].url))
          .upsert(new supercrawler.Url(r[i].url))
          .then(function () {
          });
        }

        // crawler.start();
        vm.crawler = crawler

      })

    });

    function parseHandler(domains, h) {
      if (h.url == undefined || h.url == null) {
        return false
      }
      const ext_re = /(http)?s?:?(\/\/[^"']*\.(?:png|jpg|jpeg|gif|png|svg|zip|gz|tar|rar|mp4|mov|avi)|.*%3A%2F%2F.*)/g
      if (h.url.match(ext_re)) {
        console.log("regex matched - return false")
        return false
      }
      var domain
      let d = psl.parse(extractHostname(h.url))
      if (d.subdomain && d.domain) {
        domain = d.subdomain + "." + d.domain
      } else if (d.domain) {
        domain = d.domain
      }
      console.log(domain)
      if (domains.indexOf(domain) !== -1) {
        textract.fromBufferWithMime(h.contentType, h.body, function( error, text ) {
          if (error) {
            console.log(error)
          } else {
            getMetaData({html:h.body.toString()}).then((data) => {

              var feed = {}
              feed.url = data.url
              if (data.title) {
                feed.title = data.title
              } else {
                feed.title = text.substring(0,50)
              }
              if (data.language) {
                feed.lang = data.language
              } else {
                feed.lang = "en"
              }
              if (text) {
                feed.document = text
              } else if (data.description) {
                feed.document = data.description
              }
              feed.last_modified = data.modified
              feed.feed_date = new Date()
              feed.metadata = {}
              for (var i in data) {
                if (data[i]) {
                  feed.metadata[i] = data[i]
                }
              }
              delete feed.metadata.title
              delete feed.metadata.robots
              delete feed.metadata.url
              delete feed.metadata.image
              delete feed.metadata.icon
              db_pg.addTableData(tb_, [feed], function(r) {
                console.log(r)
              })
            })
          }
        })
      }
    }
  }

  start() {
    this.crawler.start() 
    this.status_ = "running";
  }

  stop() {
    this.crawler.stop()
    this.status_ = "disabled"
  }

}

function loadConfig(callback) {
  function read_config(filename, callback_) {
    var data_ = fs.readFileSync('./config.json');
    var info = JSON.parse(data_)
    if (info.postgres_host && info.postgres_user && info.postgres_password) {
      callback_(null, info)
    } else {
      console.log('error config.json invalid')
      process.exit()
    }
  }

  read_config('./config.json', function(err, info) {
    if (err) {
      console.log(err)
      console.log('no config file found?')
      process.exit()
    } else {
      config = info;
      callback()
    }
  });
}

function extractHostname(url) {
    var hostname;
    //find & remove protocol (http, ftp, etc.) and get hostname

    if (url.indexOf("//") > -1) {
        hostname = url.split('/')[2];
    }
    else {
        hostname = url.split('/')[0];
    }

    //find & remove port number
    hostname = hostname.split(':')[0];
    //find & remove "?"
    hostname = hostname.split('?')[0];

    return hostname;
}

module.exports = Crawler;
