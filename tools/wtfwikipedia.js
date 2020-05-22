const lineReader = require('line-reader');

const wtf = require('wtf_wikipedia')

const fs = require('fs');
lines = []

function readlines(callback) {
  lineReader.eachLine('wikiids', (line) => {
    callback(parseInt(line));
  });
}

function doit() { 
  readlines(function(r) {
    wtf.fetch(parseInt(r)).then((doc) => {
      let page = {}
      page.title = doc.title();
      page.url = doc.url();
      page.lang = doc.lang();
      page.text = doc.text();
      console.log(JSON.stringify(page)+',')
    });
  });
}

function sleep(ms) {
  return new Promise(resolve => setTimeout(resolve, ms));
}

doit();

  /*
    wtf.fetch(parseInt(line)).then((doc) => {
      console.log('c')
      console.log('wtf');
      let page = {}
      page.title = doc.title();
      page.url = doc.url();
      page.lang = doc.lang();
      page.text = doc.text();
      console.log(page)
      return true;
    });
    */

  /*
      wtf.fetch(parseInt(line)).then((doc) => {
        console.log('c')
        console.log('wtf');
        let page = {}
        page.title = doc.title();
        page.url = doc.url();
        page.lang = doc.lang();
        page.text = doc.text();
        console.log(page)
        w=false;
      });
      */
  /*
      (async function exec() {
        var vm = this
        let promise = wtf.fetch(parseInt(r[i]))
        let doc = await promise;
        let page = {}
        page.title = doc.title();
        page.url = doc.url();
        page.lang = doc.lang();
        page.text = doc.text();
        await console.log(JSON.stringify(page)+',')
        wait = false;
      })();
  }
});

/*
wtf.fetch(line).then((doc) => {
  let page = {}
  page.title = doc.title();
  page.url = doc.url();
  page.lang = doc.lang();
  page.text = doc.text();
  console.log(JSON.stringify(page)+',')
});
*/

/*
var LineByLineReader = require('line-by-line'),
lr = new LineByLineReader('wikiids');

lr.on('error', function (err) {
  console.log(err)
});

lr.on('line', function (line) {
  // pause emitting of lines...
  lr.pause();

  // ...do your asynchronous line processing..
  setTimeout(function () {
    console.log(line)

    wtf.fetch(line).then((doc) => {
      console.log('wtf');
      let page = {}
      page.title = doc.title();
      page.url = doc.url();
      page.lang = doc.lang();
      page.text = doc.text();
      console.log(page)
    });

  // ...and continue emitting lines.
    lr.resume();
  }, 100);
});

lr.on('end', function () {
  // All lines are read, file is closed now.
});
*/
