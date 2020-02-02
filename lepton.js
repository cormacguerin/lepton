
var express = require('express');
var app = express();
var net = require('net');

var cookieParser = require('cookie-parser');

var bodyParser = require('body-parser');

var langparser = require('accept-language-parser');

var async = require('async');

var user = require('./api/user.js');
var data = require('./api/data.js');

user.loadExistingSessions();

const url = require('url');

const { Pool } = require('pg')

const pool = new Pool({
	user: 'postgres',
	host: 'localhost',
	database: 'index',
	password: 'kPwFWfYAsyRGZ6IomXLCypWqbmyAbK+gnKIW437QLjw=',
	port: 5432,
})

pool.on('error', (err, client) => {
  console.error('Unexpected error on idle client', err)
  process.exit(-1)
})

pool.query('SELECT NOW()', (err, res) => {
	console.log(err, res)
	//pool.end()
})

app.use(cookieParser());
app.use(bodyParser.json({limit: '10mb'}));
app.use(bodyParser.urlencoded({limit: '10mb', extended: true, parameterLimit: 1000000})); // for parsing application/x-www-form-urlencoded
app.use(bodyParser.raw({type:'image/jpeg;base64',limit: '5mb'}));
app.use(bodyParser.raw({type:'image/jpeg',limit: '50mb'}));

app.all('*', function(req, res, next) {
	res.header("Access-Control-Allow-Origin", "*");
	res.header("Access-Control-Allow-Headers", "X-Requested-With");
	next();
});

app.post('/login', function(req, res, next) {
	user.login(req, res, next);
});
app.post('/register', function(req, res, next) {
	user.register(req, res, next);
});
app.get('/api/getDatabases', function(req,res,next) {
	data.getDatabases(function(d) {
    res.json(d);
  });
});
app.get('/api/getTables', function(req,res,next) {
  var queryData = url.parse(req.url, true).query;
	data.getTables(queryData.database, function(d) {
    res.json({d});
  });
});
app.get('/api/getTableSchema', function(req,res,next) {
  var queryData = url.parse(req.url, true).query;
	data.getTableSchema(queryData.database, queryData.table, function(d) {
    res.json({d});
  });
});
app.get('/api/getUserInfo', function(req,res,next) {
	// get language and locale
	var language;
	var region;
	var languages = langparser.parse(req.headers["accept-language"]);
	console.log('req.headers["accept-language"] ' + req.headers["accept-language"]);
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
app.get('/api/addDatabase', function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
	data.addDatabase(queryData.database, function(r) {
    res.json(r);
  });
});
app.get('/api/deleteDatabase', function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
	data.deleteDatabase(queryData.database, function(r) {
    res.json(r);
  });
});
app.get('/api/createTable', function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
  if (!(queryData.database && queryData.table && queryData.column && queryData.datatype)) {
    res.json({status:'failed', message:'invalid parameters'});
    return;
  }
	data.createTable(queryData.database, queryData.table, queryData.column, queryData.datatype, function(r) {
    console.log('r');
    console.log(r);
    res.json(r);
  });
});
app.get('/api/addTable', function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
  if (!(queryData.database && queryData.table && queryData.column && queryData.datatype)) {
    res.json({status:'failed', message:'invalid parameters'});
    return;
  }
	data.addTable(queryData.database, queryData.table, queryData.column, queryData.datatype, function(r) {
    res.json(r);
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
app.post('/addTableData', function(req, res, next) {
  var field_types = [
    'serial',
    'bigserial',
    'int',
    'bigint',
    'decimal',
    'bigdecimal',
    'real',
    'date',
    'varchar_64',
    'varchar_2048',
    'text'
  ]
	var table_name;
	var fields=[];
  var data;
  var rows;
	var queryData = url.parse(req.url, true).query;
  console.log(queryData)
  // console.log(req.body)
  if (req.body) {
    if (typeof req.body === 'object') {
      data = req.body;
    } else {
      try {
        data = JSON.parse(req.body);
      } catch(e) {
        console.log(e);
        return res.json({});
      }
    }
    Object.keys(data).forEach(function(table) {
      for (var row in data[table]) {
        for (var item in data[table][row]) {
//          console.log(item + ' ' + data[table][row][item])
        }
//        console.log(' - - - ');
      }
    });
    return res.json({});
  }
});

/*
 * A function to add a document(s) to our corpus.
 */
app.post('/addData', function(req, res, next) {
	var queryData = url.parse(req.url, true).query;
	if (queryData.type == "content") {
		if (req.body) {
			var docs = req.body;
			var hasError = false;
			//multi = client.multi();
			for (var i in docs) {
				if (docs.hasOwnProperty(i)) {
					var doc_feed_lang;
					var doc_id_lang;
					if (docs[i].crawl_language == "en") {
						doc_feed_lang = "en";
						doc_id_lang = "en";
					} else if (docs[i].crawl_language == "ja") {
						doc_feed_lang = "ja";
						doc_id_lang = "ja";
					} else if (docs[i].crawl_language == "zh") {
						doc_feed_lang = "zh";
						doc_id_lang = "zh";
					} else {
						break;
					}
					var docs_table = "docs_" + doc_feed_lang;
					var docs_table_constraint = "docs_" + doc_feed_lang + "_url_key";
					(async () => {
						const client = await pool.connect()
						try {
							var insert_doc = "INSERT INTO " + docs_table + "(url, feed, lang, crawl_date)"
								+ " VALUES("
								+ "\'" + i + "\',"
								+ "\'" + JSON.stringify(docs[i]) + "\',"
								+ "\'" + docs[i].crawl_language + "\',"
								+ "NOW()) ON CONFLICT ON CONSTRAINT " + docs_table_constraint + " DO UPDATE SET feed = "
								+ "\'" + JSON.stringify(docs[i]) + "\', lang = "
								+ "\'" + docs[i].crawl_language + "\', crawl_date = NOW() WHERE " + docs_table + ".url = "
								+ "\'" + i + "\';";
							// console.log(insert_doc);
							const reply = await client.query(insert_doc);
							for (r in reply) {
								console.log("r : " + r);
							}
							console.log("i : " + i);
						} finally {
							client.release()
						}
					})().catch(
						e => {
							console.log(e.stack);
							hasError = true;
					})
				}
			}
			if (hasError) {
				res.status(503);
				return res.json({
						"status":"failed",
						"error":e.stack
				});
			} else {
				res.status(200);
				res.json({
						"status":"successful"
				});
			}
		}
	}
});


/*
 * A function to add a document(s) to our corpus.
 */
app.post('/addDocument', function(req, res, next) {
	var queryData = url.parse(req.url, true).query;
	if (queryData.type == "content") {
    print(req)
		if (req.body) {
			var docs = req.body;
			var hasError = false;
			//multi = client.multi();
			for (var i in docs) {
				if (docs.hasOwnProperty(i)) {
					var doc_feed_lang;
					var doc_id_lang;
					if (docs[i].crawl_language == "en") {
						doc_feed_lang = "en";
						doc_id_lang = "en";
					} else if (docs[i].crawl_language == "ja") {
						doc_feed_lang = "ja";
						doc_id_lang = "ja";
					} else if (docs[i].crawl_language == "zh") {
						doc_feed_lang = "zh";
						doc_id_lang = "zh";
					} else {
						break;
					}
					var docs_table = "docs_" + doc_feed_lang;
					var docs_table_constraint = "docs_" + doc_feed_lang + "_url_key";
					(async () => {
						const client = await pool.connect()
						try {
							var insert_doc = "INSERT INTO " + docs_table + "(url, feed, lang, crawl_date)"
								+ " VALUES("
								+ "\'" + i + "\',"
								+ "\'" + JSON.stringify(docs[i]) + "\',"
								+ "\'" + docs[i].crawl_language + "\',"
								+ "NOW()) ON CONFLICT ON CONSTRAINT " + docs_table_constraint + " DO UPDATE SET feed = "
								+ "\'" + JSON.stringify(docs[i]) + "\', lang = "
								+ "\'" + docs[i].crawl_language + "\', crawl_date = NOW() WHERE " + docs_table + ".url = "
								+ "\'" + i + "\';";
							// console.log(insert_doc);
							const reply = await client.query(insert_doc);
							for (r in reply) {
								console.log("r : " + r);
							}
							console.log("i : " + i);
						} finally {
							client.release()
						}
					})().catch(
						e => {
							console.log(e.stack);
							hasError = true;
					})
				}
			}
			if (hasError) {
				res.status(503);
				return res.json({
						"status":"failed",
						"error":e.stack
				});
			} else {
				res.status(200);
				res.json({
						"status":"successful"
				});
			}
		}
	}
});

/*
 * A function to add a document(s) to our corpus.
 */
app.get('/search', function(req, res, next) {
	try {
		var queryData = url.parse(req.url, true).query;
		var socket = new net.Socket();
		if (!queryData.query) {
			res.json({"error":"no query"});
			return;
		} else {
			execute(req,res);
		}
	} catch(e) {
		res.send({"error":"\""+e+"\""});
		console.log(e);
		return;
	}
});

function execute(req, res) {
		var queryData = url.parse(req.url, true).query;
		var socket = new net.Socket();
		socket.connect(3333, '127.0.0.1', function() {
			var data_length = queryData.query.length;
			var header = "length:" + ('000000' + data_length).substr(data_length.toString().length) + ":";
			socket.write(header.concat(queryData.query),'utf8', function(r) {
				console.log(r);
			});
			socket.end();
		});
		var packet = "";
		socket.on('data', (data) => {
			packet += data.toString();
			// console.log('packet data');
			// console.log(packet);
			socket.end();
		});
		socket.on('end', () => {
			res.json(packet);
			console.log('disconnected from server');
		});
		socket.on('unhandledRejection', (error, promise) => {
			res.send({"error":+error+"\""});
			console.log(error);
			return;
		});
		socket.on('uncaughtException', (error, promise) => {
			res.send({"error":+error+"\""});
			console.log(error);
			return;
		});
		socket.on('error', (error, promise) => {
			res.send({"error":+error+"\""});
			console.log(error);
			return;
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

app.use('/data/', express.static(__dirname + '/vue-app/dist/'));
app.use('/visualize/', express.static(__dirname + '/vue-app/dist/'));

// web root
app.use('/', express.static(__dirname + '/vue-app/dist/'));

// start the server.
var server = app.listen(process.env.PORT || 3000, function () {
	console.log('Web app listening on port 3000!')
});
