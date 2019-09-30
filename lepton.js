
var express = require('express');
var app = express();
var net = require('net');

var cookieParser = require('cookie-parser');

var bodyParser = require('body-parser');

var async = require('async');

//var request = require('./api/restrequest');

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
app.use(bodyParser.json({limit: '1mb'}));
app.use(bodyParser.urlencoded({limit: '1mb',extended: true })); // for parsing application/x-www-form-urlencoded
app.use(bodyParser.raw({type:'image/jpeg;base64',limit: '5mb'}));
app.use(bodyParser.raw({type:'image/jpeg',limit: '5mb'}));

app.all('*', function(req, res, next) {
	res.header("Access-Control-Allow-Origin", "*");
	res.header("Access-Control-Allow-Headers", "X-Requested-With");
	next();
});

/*
 * A function to add a document(s) to our corpus.
 */
app.post('/addDocument', function(req, res, next) {
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
						}
					)
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
	console.log('search');
	try {
	console.log('in try');
	var queryData = url.parse(req.url, true).query;
	var socket = new net.Socket();
	console.log('deb 1');
	if (!queryData.query) {
		res.json({"error":"no query"});
		return;
	}
	console.log('deb 2');
	console.log('req in ' + queryData.query);
	socket.connect(3333, '127.0.0.1', function() {
		console.log('Connected');
		console.log("write : " + queryData.query);
		var data_length = queryData.query.length;
		var header = "length:" + ('000000' + data_length).substr(data_length.toString().length) + ":"; 
		console.log("header : " + header);
		console.log("socket.bufferSize : " + socket.bufferSize);
		socket.write(header.concat(queryData.query),'utf8', function(r) {
			console.log(r);
		});
		socket.end();
	});
	/*
	const client = net.createConnection({ port: 3333 }, () => {
		console.log('connected to server!');
		console.log("write : " + queryData.query);
		client.write(queryData.query + '\r\n');
	});*/
	var packet = "";
	socket.on('data', (data) => {
		packet += data.toString();
		console.log('data received');
		console.log(packet);
		socket.end();
	});
	socket.on('end', () => {
		// res.send({"error":"disconnected"});
		console.log('res.json packet');
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
	} catch(e) { 
		res.send({"error":"\""+e+"\""});
		console.log(e);
		return;
	}
});

// web root
app.use('/', express.static(__dirname + '/web-app/build/'));

// start the server.
var server = app.listen(process.env.PORT || 3000, function () {
	console.log('Web app listening on port 3000!')
});
