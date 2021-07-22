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
          let d = psl.get(extractHostname(r[i].url))
          //let d = urlMod.parse(r[i].url).hostname
          domains.push(d)
          domains.push('www.' + d)
        }
        vm.domains = domains
        console.log(domains)

        crawler.addHandler("text/html", supercrawler.handlers.htmlLinkParser({
          // Restrict discovered links to the following hostnames.
          // hostnames: domains,         
          urlFilter: function(url) {
            let d = psl.get(extractHostname(url))
            if (domains.indexOf(d) !== -1) {
              // console.log('index url with domain ' + d)
              return true
            } else {
              // console.log('do NOT index url with domain ' + d)
              return false
            }
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
      let d = psl.get(extractHostname(h.url))
      // let d = urlMod.parse(h.url).hostname;
      console.log(d)
      if (domains.indexOf(d) != -1) {
        console.log('go')
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
