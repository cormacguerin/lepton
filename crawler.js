var supercrawler = require("supercrawler");
var psl = require('psl');
var db_pg = {}

const textract = require('textract')

const pg = require('./api/postgres.js');

const fs = require('fs')

const buffer = require('buffer')

const getMetaData = require('metadata-scraper')

exports.init = async function(v, callback) {

  await loadConfig(function(r) {
    console.log('config.json loaded')
  })

  db_pg['admin'] = new pg(config)

  var kids_config = {}
  kids_config['postgres_database'] = '3_kiddycharts'
  kids_config['postgres_password'] = config.postgres_password
  kids_config['postgres_host'] = config.postgres_host
  kids_config['postgres_user'] = config.postgres_user
  db_pg['kids'] = new pg(kids_config)

  return new supercrawler.Crawler({
    // By default, Supercrawler uses a simple FIFO queue, which doesn't support
    // retries or memory of crawl state. For any non-trivial crawl, you should
    // create a database. Provide your database config to the constructor of
    // DbUrlList.
    urlList: new supercrawler.DbUrlList({
      db: {
        database: config.postgres_database,
        username: config.postgres_user,
        password: config.postgres_password,
        sequelizeOpts: {
          dialect: "postgres",
          host: config.postgres_host
        }
      }
    }),
    // Tme (ms) between requests
    interval: 1000,
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

this.init().then(crawler=> {

  // Get "Sitemaps:" directives from robots.txt
  crawler.addHandler(supercrawler.handlers.robotsParser());

  // Crawl sitemap files and extract their URLs.
  crawler.addHandler(supercrawler.handlers.sitemapsParser());

  db_pg['admin'].getAllCrawlerUrls(function(r) {
    var domains=[]
    for (var i=0; i < r.length; i++) {
      let d = psl.get(extractHostname(r[i].url))
      domains.push(d)
      domains.push('www.' + d)
    }

    console.log('domains')
    console.log(domains)

    crawler.addHandler("text/html", supercrawler.handlers.htmlLinkParser({
      // Restrict discovered links to the following hostnames.
      hostnames: domains
    }));

    crawler.addHandler(["text/plain", "text/html"], parseHandler);

    for (var i=0; i < r.length; i++) {
      crawler.getUrlList()
      .insertIfNotExists(new supercrawler.Url(r[i].url))
      .then(function () {
      });
      crawler.start();
    }

    console.log('start')
    crawler.start();

  })

});

function parseHandler(h) {
  textract.fromBufferWithMime(h.contentType, h.body, function( error, text ) {
    if (error) {
      console.log(error)
    } else {
      getMetaData({html:h.body.toString()}).then((data) => {
        feed = {}
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
        db_pg['kids'].addTableData('kiddychartsnetwork', [feed], function(r) {
          console.log(r)
        })
      })
    }
  })
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

