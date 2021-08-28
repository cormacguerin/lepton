
var express = require('express');
var app = express();
var net = require('net');

var cookieParser = require('cookie-parser');

var bodyParser = require('body-parser');

var langparser = require('accept-language-parser');

var async = require('async');

var user = require('./api/user.js');

var data = require('./api/data.js');

var crawler = require('./crawler.js');

var crypto = require('crypto');

var queryServers = {}

var crawlers = {}

const compression = require('compression');

const url = require('url');

const v = process.argv.slice(2)[0]
// const z = process.argv.slice(2)[1]

data.init(v, function(pg_admin) {
  console.log('lepton - data.init response')
  user.init(pg_admin)
})

app.use(compression());
app.use(cookieParser());
app.use(bodyParser.json({limit: '100mb'}));
app.use(bodyParser.urlencoded({limit: '100mb', extended: true, parameterLimit: 1000000})); // for parsing application/x-www-form-urlencoded
app.use(bodyParser.raw({type:'image/jpeg;base64',limit: '5mb'}));
app.use(bodyParser.raw({type:'image/jpeg',limit: '50mb'}));
 //app.use(cors())

app.all('*', function(req, res, next) {
  res.header("Access-Control-Allow-Origin", "*");
  res.header("Access-Control-Allow-Headers", "X-Requested-With");
  res.header("Access-Control-Allow-Headers", "Authorization");
  next();
});

app.post('/login', function(req, res, next) {
  user.login(req, res, next);
});
app.post('/register', function(req, res, next) {
  user.register(req, res, next);
});
app.get('/logout', user.authorize, function(req, res, next) {
  user.logout(req, res, next);
});
app.get('/api/getDatabases', user.authorize, function(req,res,next) {
  data.getDatabases(req.user_id,function(d) {
    res.json(d);
  });
});
/*
app.get('/api/getTables', user.authorize, function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
    data.getTables(queryData.database, function(d) {
    res.json(d);
  });
});
*/
app.get('/api/getTableSchema', user.authorize, function(req,res,next) {
  var queryData = url.parse(req.url, true).query;
  data.getTableSchema(req.user_id, queryData.database, queryData.table, function(d) {
    res.json({d});
  });
});
app.get('/api/getIndexingInfo', user.authorize, function(req,res,next) {
  var queryData = url.parse(req.url, true).query;
  data.getIndexTables(req.user_id, function(d) {
    res.json(d);
  });
});
app.get('/api/getServingInfo', user.authorize, function(req,res,next) {
  var queryData = url.parse(req.url, true).query;
  data.getServingTables(req.user_id, function(d) {
    getStats(function(s) {
      if (s) {
        for (var i in d) {
          if (s[req.user_id + "_" +d[i].database]) {
            if (s[req.user_id + "_" +d[i].database][d[i].table]) {
              d[i].terms = s[req.user_id + "_" +d[i].database][d[i].table].terms;
              d[i].status = s[req.user_id + "_" +d[i].database][d[i].table].status;
              d[i].loaded = s[req.user_id + "_" +d[i].database][d[i].table].loaded;
            }
          }
        }
      }
      res.json(d);
    });
  });
});
app.get('/api/getUserInfo', user.authorize, function(req,res,next) {
  // get language and locale
  var language;
  var region;
  var languages = langparser.parse(req.headers["accept-language"]);
  if (languages[0]) {
    if (languages[0].code) {
      language=languages[0].code;
    }
    if (languages[0].region) {
      region=languages[0].region.toLowerCase();
    }
  }
  if (languages[1]) {
    if (languages[1].code && !language) {
      language=languages[1].code;
    }
    if (languages[1].region && !region) {
      region=languages[1].region.toLowerCase();
    }
  }
  req.language = language;
  req.region = region;
  user.authorize(req, res, function() {
    user.getUserInfo(req, res);
  });
});
app.get('/api/addDatabase', user.authorize, function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
  data.addDatabase(req.user_id, queryData.database, function(r) {
    res.json(r);
  });
});
app.get('/api/deleteTable', user.authorize, function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
  if (!(queryData.database && queryData.table && queryData.type)) {
    res.json({status:'failed', message:'invalid parameters'});
    return;
  }
  data.deleteTable(req.user_id, queryData.database, queryData.table, queryData.type, function(r) {
    res.json(r);
  });
});
app.get('/api/deleteDatabase', user.authorize, function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
  data.deleteDatabase(req.user_id, queryData.database, function(r) {
    res.json(r);
  });
});
app.get('/api/createTable', user.authorize, function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
  if (!(queryData.database && queryData.table && queryData.column && queryData.datatype)) {
    res.json({status:'failed', message:'invalid parameters'});
    return;
  }
  data.createTable(req.user_id, queryData.database, queryData.table, queryData.column, queryData.datatype, function(r) {
    res.json(r);
  });
});
app.get('/api/createSearchTable', user.authorize, function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
  if (!(queryData.database && queryData.table)) {
    res.json({status:'failed', message:'invalid parameters'});
    return;
  }
  data.createSearchTable(req.user_id, queryData.database, queryData.table, function(r) {
    res.json(r);
  });
});
app.get('/api/createDataSetTable', user.authorize, function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
  if (!(queryData.database && queryData.table && queryData.query)) {
    res.json({status:'failed', message:'invalid parameters'});
    return;
  }
  data.createDataSetTable(req.user_id, queryData.database, queryData.table, queryData.query, function(r) {
    res.json(r);
  });
});
app.get('/api/deleteDataSetTable', user.authorize, function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
  if (!(queryData.database && queryData.table)) {
    res.json({status:'failed', message:'invalid parameters'});
    return;
  }
  data.deleteDataSetTable(req.user_id, queryData.database, queryData.table, function(r) {
    res.json(r);
  });
});
app.get('/api/addTableColumn', user.authorize, function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
  if (!(queryData.database && queryData.table && queryData.column && queryData.datatype)) {
    res.json({status:'failed', message:'invalid parameters'});
    return;
  }
  data.addTableColumn(req.user_id, queryData.database, queryData.table, queryData.column, queryData.datatype, function(r) {
    res.json(r);
  });
});
app.get('/api/updateTableColumn', user.authorize, function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
  if (!(queryData.database && queryData.table && queryData.column && queryData.editColumn && queryData.datatype && queryData.editDatatype)) {
    res.json({status:'failed', message:'invalid parameters'});
    return;
  }
  data.updateTableColumn(req.user_id, queryData.database, queryData.table, queryData.column, queryData.editColumn, queryData.datatype, queryData.editDatatype, function(r) {
    res.json(r);
  });
});
app.get('/api/deleteColumn', user.authorize, function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
  if (!(queryData.database && queryData.table && queryData.column)) {
    res.json({status:'failed', message:'invalid parameters'});
    return;
  }
  data.deleteTableColumn(req.user_id, queryData.database, queryData.table, queryData.column, function(r) {
    res.json(r);
  });
});
app.get('/api/addModel', user.authorize, function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
  var id = queryData.id;
  var model = queryData.model;
  var language = queryData.language;
  var program = queryData.program;
  var dataset = queryData.dataset;
  if (parseInt(id)) {
    data.saveModel(req.user_id, id, language, model, program, dataset, function(r) {
      res.json(r);
    });
  } else {
    data.addModel(req.user_id, language, model, function(r) {
      res.json(r);
    });
  }
});
app.get('/api/getModels', user.authorize, function(req,res,next) {
  var queryData = url.parse(req.url, true).query;
  data.getModels(req.user_id, function(d) {
    res.json(d);
  });
});
app.get('/api/createMemoryTable', user.authorize, function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
  if (!(queryData.database && queryData.table && queryData.column && queryData.datatype)) {
    res.json({status:'failed', message:'invalid parameters'});
    return;
  }
  data.createMemoryTable(req.user_id, queryData.database, queryData.table, queryData.column, queryData.datatype, function(r) {
    execute({'query':'toggle_serving','database':queryData.database,'table':queryData.table,'action':queryData.serving}, 3333, function(r_) {
      res.json(r);
    });
  });
});
app.get('/getDataset', user.authorize, function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
  var table = queryData.table;
  var database = queryData.database;

  // handle user or token request
  if (req.user_id) {
    database = req.user_id + "_" + queryData.database;
  } else {
    var queryData = url.parse(req.url, true).query;
    // validate scope
    var access = false;
    for (var i in req.scope) {
      if (queryData.database === req.scope[i].database) {
        if (req.scope[i].table) {
          if (queryData.table === req.scope[i].table) {
            access = true;
            database = req.scope[i]._database;
          }
        } else {
          access = true;
          database = req.scope[i]._database;
        }
      }
    }
    if (access != true) {
      res.status(403);
      return res.json({});
    }
  }
  if (table && database) {
    data.getDataSet(database, table, function(r) {
      res.json(r);
    });
  } else {
    res.json({status:'failed', message:'invalid parameters'});
    return;
  }
});
app.get('/api/setFTS', user.authorize, function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
  if (!(queryData.database && queryData.table && queryData.column && queryData.fts)) {
    res.json({status:'failed', message:'invalid parameters'});
    return;
  }
  data.setFTS(req.user_id, queryData.database, queryData.table, queryData.column, queryData.fts, function(r) {
    res.json(r);
  });
});
app.get('/api/addServingColumn', user.authorize, function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
  if (!(queryData.database && queryData.table && queryData.column)) {
    res.json({status:'failed', message:'invalid parameters'});
    return;
  }
  data.addServingColumn(req.user_id, queryData.database, queryData.table, queryData.column, function(r) {
    res.json(r);
  });
});
app.get('/api/removeServingColumn', user.authorize, function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
  if (!(queryData.database && queryData.table && queryData.column)) {
    res.json({status:'failed', message:'invalid parameters'});
    return;
  }
  data.removeServingColumn(req.user_id, queryData.database, queryData.table, queryData.column, function(r) {
    res.json(r);
  });
});
app.get('/api/setServing', user.authorize, function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
  if (!(queryData.database && queryData.table && queryData.serving)) {
    res.json({status:'failed', message:'invalid parameters'});
    return;
  }
  data.setServing(req.user_id, queryData.database, queryData.table, queryData.serving, function(r) {
    execute({'query':'toggle_serving','database':req.user_id + '_' + queryData.database,'table':queryData.table,'action':queryData.serving}, 3333, function(r_) {
      res.json(r);
    });
  });
});
app.get('/api/setFTSDisplayField', user.authorize, function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
  if (!(queryData.database && queryData.table && queryData.display_field)) {
    res.json({status:'failed', message:'invalid parameters'});
    return;
  }
  data.setFTSDisplayField(req.user_id, queryData.database, queryData.table, queryData.display_field, function(r) {
    res.json(r);
  });
});
app.get('/api/addCrawlerUrl', user.authorize, function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
  data.addCrawlerUrl(req.user_id, queryData.database, queryData.table, queryData.url, function(r) {
    res.json(r);
  });
});
app.get('/api/deleteCrawlerUrl', user.authorize, function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
  data.deleteCrawlerUrl(req.user_id, queryData.database, queryData.table, queryData.url, function(r) {
    res.json(r);
  });
});
app.get('/api/getCrawlerUrls', user.authorize, function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
  data.getCrawlerUrls(req.user_id, queryData.database, function(r) {
    res.json(r);
  });
});
app.get('/api/getCrawlerStatus', user.authorize, function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
  let this_crawler = crypto.createHash('md5').update(req.user_id + queryData.database + queryData.table).digest('hex')
  if (!crawlers[this_crawler]) {
    crawlers[this_crawler] = new crawler(req.user_id + "_" + queryData.database, queryData.table)
  }
  if (queryData.action == 'start') {
    crawlers[this_crawler].start()
  }
  if (queryData.action == 'stop') {
    crawlers[this_crawler].stop()
  }
  setTimeout(()=> {
    res.json({'status':crawlers[this_crawler].status_})
  }, 3000);
});
app.get('/api/runQuery', user.authorize, function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
  if (!(queryData.database && queryData.query)) {
    res.json({status:'failed', message:'invalid parameters'});
    return;
  }
  data.runQuery(req.user_id, queryData.database, queryData.query, function(r) {
    res.json(r);
  });
});
app.get('/api/addChart', user.authorize, function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
  if (!(queryData.database && queryData.dataset && queryData.name && queryData.chart && queryData.data)) {
    res.json({status:'failed', message:'invalid parameters'});
    return;
  }
  data.addChart(req.user_id, queryData.database, queryData.dataset, queryData.name, queryData.chart, queryData.data, function(r) {
    res.json(r);
  });
});
app.get('/api/getMyCharts', user.authorize, function(req, res, next) {
  data.getMyCharts(req.uer_id, function(r) {
    res.json(r);
  });
});
app.get('/api/getChartById', user.authorize, function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
  if (!(queryData.chart_id)) {
    res.json({status:'failed', message:'invalid parameters'});
    return;
  }
  data.getChartById(req.user_id, queryData.chart_id, function(r) {
    res.json(r);
  });
});

/*
 * Function to generate an API key
 */
app.get('/api/generateApiKey', user.authorize, function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
  const key = user.genApiKey();
  res.status(200);
  res.send(key);
});
/*
 * Function to get a users api keys
 */
app.get('/api/getApiKeys', user.authorize, function(req, res, next) {
  data.getApiKeys(req.user_id, function(r) {
    res.json(r);
  });
});
/*
 * Function to add an API key
 */
app.get('/api/addApiKey', user.authorize, function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
  if (!(queryData.key_name && queryData.key)) {
    res.json({status:'failed', message:'invalid parameters'});
    return;
  }
  data.addApiKey(req.user_id, queryData.key_name, queryData.key, function(r) {
    res.json(r);
  });
});
/*
 * Function toan API key scope
 */
app.get('/api/addApiScope', user.authorize, function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
  if (!(queryData.key_id && queryData.api && queryData.database)) {
    res.json({status:'failed', message:'invalid parameters'});
    return;
  }
  data.addApiScope(req.user_id, queryData.key_id, queryData.api, queryData.database, queryData.table, function(r) {
    res.json(r);
  });
});
app.get('/api/deleteApiKey', user.authorize, function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
  if (!(queryData.key_id)) {
    res.json({status:'failed', message:'invalid parameters'});
    return;
  }
  data.deleteApiKey(req.user_id, queryData.key_id, function(r) {
    res.json(r);
  });
});
app.get('/api/deleteApiScope', user.authorize, function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
  if (!(queryData.key_id && queryData.api_scope && queryData.api_database)) {
    res.json({status:'failed', message:'invalid parameters'});
    return;
  }
  data.deleteApiScope(req.user_id, queryData.key_id, queryData.api_scope, queryData.api_database, queryData.api_table, function(r) {
    res.json(r);
  });
});
app.get('/api/image', user.authorize, function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
  if (!(queryData.key_id && queryData.api_scope && queryData.api_database)) {
    res.json({status:'failed', message:'invalid parameters'});
    return;
  }
  const { spawn } = require('child_process');
  const pyProg = spawn('python', ['./../pypy.py']);

  pyProg.stdout.on('data', function(data) {

      console.log(data.toString());
      res.write(data);
      res.end('end');
  });
});

/*
 *	A table definition. example data
 * 	{
 * 		"name": "mytable",
 *		"fields": [FIELD,FIELD],
 *		"pkey": "some FIELD name"
 * 	}
 *
 * 	where fields a json array of field objects
 * 	field objects are columns names which struct as follows
 *
 *	{
 *		"name": "myfield",
 *		"type": TYPE,
 *	}
 *
 *	where TYPE is one of
 *	 - text
 *	 - integer
 *	 - real
 *	 - datetime
 *	 - key_value[KEY_VALUE,KEY_VALUE]
 *
 *	{
 *		"name": "mykey",
 *		"value": VALUE,
 *	}
 *
 *	where VALUE is one of
 *	 - text
 *	 - integer
 *	 - real
 *	 - datetime
 *	i.e KEY_VALUE is a non nestible field type.
 *
 */
app.post('/addTableData', user.authorize, function(req, res, next) {
  var table_name;
  var data_;
  var database;
  var queryData = url.parse(req.url, true).query;
  if (!queryData.database) {
    res.status(403);
    return res.json({});
  }
  // authorize scope
  var access = false;
  for (var i in req.scope) {
    if (queryData.database === req.scope[i].database) {
      if (req.scope[i].table) {
        if (queryData.table === req.scope[i].table) {
          access = true;
          database = req.scope[i]._database;
        }
      } else {
        access = true;
        database = req.scope[i]._database;
      }
    }
  }
  if (access === true) {
    data.checkTableExists(database, queryData.table, function(r) {
      if (r === true) {
        if (req.body) {
          if (typeof req.body === 'object') {
            data_ = req.body;
          } else {
            try {
              data_ = JSON.parse(req.body);
            } catch(e) {
              console.log(e);
              return res.json({});
            }
          }
          data.addTableData(database, queryData.table, data_, function(e,r) {
            if (e) {
              res.json({'message':e.message,'error':e});
            } else {
              res.json({r});
            }
          });
        }
      } else {
        res.status(404);
        return res.json({'message':'table not found'});
      }
    });
  } else {
    res.status(403);
    return res.json({});
  }
});

/*
 * Search for a document
 * TODO there is a lot of duplication and stuff here that could be cleaned up
 */
app.get('/search', user.authorize, function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
  var database;

  if (!queryData.query && !(queryData.filter || queryData['filter[]'])) {
    res.json({"error":"no query provided"});
    return;
  } else {
    table = queryData.table;
  }
  var queryData = url.parse(req.url, true).query;
  if (!queryData.database) {
    res.json({"error":"no database provided"});
    return;
  } else {
    database = queryData.database;
  }
  var queryData = url.parse(req.url, true).query;
  if (!queryData.table) {
    res.json({"error":"no table provided"});
    return;
  } else {
    table = queryData.table;
  }
  var queryData = url.parse(req.url, true).query;
  // handle user or token request
  if (req.user_id) {
    database = req.user_id + "_" + queryData.database;
  } else {
    var queryData = url.parse(req.url, true).query;
    // validate scope
    var access = false;
    for (var i in req.scope) {
      if (queryData.database === req.scope[i].database) {
        if (req.scope[i].table) {
          if (queryData.table === req.scope[i].table) {
            access = true;
            database = req.scope[i]._database;
          }
        } else {
          access = true;
          database = req.scope[i]._database;
        }
      }
    }
    if (access != true) {
      res.status(403);
      return res.json({});
    }
  }
  try {
    queryData.type = "search";
    // vuejs adds stupid brackets on array so we need to check.
    if (queryData["pages[]"]) {
        queryData.pages = queryData["pages[]"];
    }
    if (queryData["filter[]"]) {
        queryData.filter = queryData["filter[]"];
    }
    var socket = new net.Socket();
    if (Object.keys(queryServers).length === 0) {
      doGetStats(database,table,queryData,res);
    } else if (queryServers[database]) {
      if (queryServers[database][table]) {
        // this might get slow if we scale up so need some better way to do it.
        // I'm thinking we just return the basic candidates and let the user define queries separately.
        var user_id;
        if (req.user_id) {
          user_id = req.user_id;
        } else if (req.api_key_owner) {
          user_id = req.api_key_owner;
        } else {
          console.log("should never get here");
          res.status(403);
          return res.json({});
        }
        data.getServingColumns(user_id, database, table, function(c) {
          queryData.columns = c.join();
          execute(queryData,queryServers[database][table].port,function(r) {
            console.log(r)
            res.json(r);
          });
        });
      } else {
        doGetStats(database,table,queryData,res);
      }
    } else {
      doGetStats(database,table,queryData,res);
    }
  } catch(e) {
    res.send({"error":"\""+e+"\""});
    console.log(e);
    return;
  }
});

/*
 * Search for a document
 * TODO there is a lot of duplication and stuff here that could be cleaned up
 */
app.get('/suggest', user.authorize, function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
  var database;

  if (!queryData.query) {
    res.json({"error":"no query provided"});
    return;
  } else {
    table = queryData.table;
  }
  if (!queryData.database) {
    res.json({"error":"no database provided"});
    return;
  } else {
    database = queryData.database;
  }
  if (!queryData.table) {
    res.json({"error":"no table provided"});
    return;
  } else {
    table = queryData.table;
  }
  // handle user or token request
  if (req.user_id) {
    database = req.user_id + "_" + queryData.database;
  } else {
    // validate scope
    var access = false;
    for (var i in req.scope) {
      if (queryData.database === req.scope[i].database) {
        if (req.scope[i].table) {
          if (queryData.table === req.scope[i].table) {
            access = true;
            database = req.scope[i]._database;
          }
        } else {
          access = true;
          database = req.scope[i]._database;
        }
      }
    }
    if (access != true) {
      res.status(403);
      return res.json({});
    }
  }
  try {
    queryData.type = "suggest";
    var socket = new net.Socket();
    if (Object.keys(queryServers).length === 0) {
      doGetStats(database,table,queryData,res);
    } else if (queryServers[database]) {
      if (queryServers[database][table]) {
        execute(queryData,queryServers[database][table].port,function(r) {
          res.json(r);
        });
      } else {
        doGetStats(database,table,queryData,res);
      }
    } else {
      doGetStats(database,table,queryData,res);
    }
  } catch(e) {
    res.send({"error":"\""+e+"\""});
    console.log(e);
    return;
  }
});

function doGetStats(database,table,queryData,res) {
  getStats(function() {
    if (queryServers[database]) {
      if (queryServers[database][table]) {
        execute(queryData,queryServers[database][table].port,function(r) {
          res.json(r);
        });
      } else {
        return res.json({});
      }
    } else {
      return res.json({});
    }
  })
}

app.get('/manage', function(req, res, next) {
  try {
    var queryData = url.parse(req.url, true).query;
    var socket = new net.Socket();
    if (!queryData.query) {
      res.json({"error":"no query"});
      return;
    } else {
    }
  } catch(e) {
    res.send({"error":"\""+e+"\""});
    console.log(e);
    return;
  }
});

function getStats(callback) {
  execute({'query':'stats'}, 3333, function(r) {
    if (r['error']) {
      return callback();
    }
    if (r.servers) {
      r.servers.forEach(function(s) {
        if (!queryServers[s.database]) {
            queryServers[s.database] = {}
        }
        queryServers[s.database][s.table] = s;
      });
    }
    callback(queryServers);
  });
}

function toggleServing(database, table, callback) {
  execute({'query':'enable','database':database,'table':table}, 3333, function(r) {
    callback(r);
  });
}

function execute(queryData, port, callback) {
  var tmpQuery = {}
  queryData.lang = "en";
  internalQuery = JSON.stringify(queryData);

  var socket = new net.Socket();
  socket.connect(port, '127.0.0.1', function() {
    // var data_length = Array.from(internalQuery).length;
    var data_length = Buffer.byteLength(internalQuery, 'utf8')
    var header = "length:" + ('000000' + data_length).substr(data_length.toString().length) + ":";
    socket.write(header.concat(internalQuery),'utf8', function(r) {
      //console.log('socket.write');
      //console.log(r);
    });
    socket.end();
  });
  var packet = "";
  socket.on('data', (data) => {
    packet += data.toString();
    socket.end();
  });
  socket.on('end', () => {
    if (packet) {
      var result = {};
      try {
        result = JSON.parse(packet);
      } catch(e) {
        console.log(e);
      }
      callback(result);
    } else {
      callback('{}');
    }
  });
  socket.on('unhandledRejection', (error, promise) => {
    callback({"error":error});
  });
  socket.on('uncaughtException', (error, promise) => {
    callback({"error":error});
  });
  socket.on('error', (error, promise) => {
    callback({"error":error});
  });
}

/*
app.get('/', function(req, res, next) {
    try {
        var queryData = url.parse(req.url, true).query;
        if (!queryData.query) {
            next();
        } else {
            execute(req,res);
        }
    } catch(e) {
        res.send({"error":"\""+e+"\""});
        console.log(e);
        return;
    }
});
*/

app.use('/database/', express.static(__dirname + '/vue-app/dist/'));
app.use('/dashboard/', express.static(__dirname + '/vue-app/dist/'));
app.use('/apikeys/', express.static(__dirname + '/vue-app/dist/'));
app.use('/indexing/', express.static(__dirname + '/vue-app/dist/'));
app.use('/serving/', express.static(__dirname + '/vue-app/dist/'));
app.use('/models/', express.static(__dirname + '/vue-app/dist/'));
app.use('/inference/', express.static(__dirname + '/vue-app/dist/'));
app.use('/insights/', express.static(__dirname + '/vue-app/dist/'));
app.use('/crawler/', express.static(__dirname + '/vue-app/dist/'));
app.use('/assets/', express.static(__dirname + '/assets/'));
// web root
app.use('/', express.static(__dirname + '/vue-app/dist/'));

// start the server.
var server = app.listen(process.env.PORT || 3000, function () {
  console.log('Web app listening on port 3000!')
});
