
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
	password: 'FSa7+aE1vztVIUZiwAt03d4O7YO2Acm6YVyrGloDZKk=',
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
app.use(bodyParser.urlencoded({ extended: true })); // for parsing application/x-www-form-urlencoded
app.use(bodyParser.raw({type:'image/jpeg;base64',limit: '5mb'}));
app.use(bodyParser.raw({type:'image/jpeg',limit: '5mb'}));

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
                    var doc_feed_lang = "unknown";
                    var doc_id_lang = "unknown";
					switch (docs[i].crawl_language) {
						case "en":
							doc_feed_lang = "en";
							doc_id_lang = "en";
						case "ja":
							doc_feed_lang = "ja";
							doc_id_lang = "ja";
					}
					(async () => {
						const client = await pool.connect()
						try {
							var insert_doc = "INSERT INTO docs(url, feed, lang, crawl_date)"
								+ " VALUES("
								+ "\'" + i + "\',"
								+ "\'" + JSON.stringify(docs[i]) + "\',"
								+ "\'" + docs[i].crawl_language + "\',"
								+ "NOW()) ON CONFLICT ON CONSTRAINT docs_url_key DO UPDATE SET feed = "
								+ "\'" + JSON.stringify(docs[i]) + "\', lang = "
								+ "\'" + docs[i].crawl_language + "\', crawl_date = NOW() WHERE docs.url = "
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
	try {
	var queryData = url.parse(req.url, true).query;
	var client = new net.Socket();
	client.connect(3333, '127.0.0.1', function() {
		console.log('Connected');
		console.log("write : " + queryData.query);
		var data_length = queryData.query.length;
		var header = "length:" + ('000000' + data_length).substr(data_length.toString().length) + ":"; 
		console.log("header : " + header);
		client.write(header.concat(queryData.query),'utf8', function(r) {
			console.log(r);
		});
		client.end();
	});
		/*
	const client = net.createConnection({ port: 3333 }, () => {
		console.log('connected to server!');
		console.log("write : " + queryData.query);
		client.write(queryData.query + '\r\n');
	});*/
	client.on('data', (data) => {
		console.log('data received');
		console.log(data.toString());
		res.send(data.toString());
		client.end();
	});
	client.on('end', () => {
		console.log('disconnected from server');
	});
	client.on('unhandledRejection', (error, promise) => {
		console.log(error);
	});
	client.on('uncaughtException', (error, promise) => {
		console.log(error);
	});
	client.on('error', (error, promise) => {
		console.log(error);
	});
	} catch(e) { 
		console.log(e);
	}
});

/*
 * A function to add a document(s) to our corpus.
 */
/*
app.post('/addVocabulary', function(req, res, next) {
	var queryData = url.parse(req.url, true).query;
	if (queryData.type == "content") {
		if (req.body){
			var docs = req.body;
			//multi = client.multi();
			for (var i in docs) {
				if (docs.hasOwnProperty(i)) {
	  				//multi.hset("vocab_feed", i, JSON.stringify(docs[i]));
					//multi.sadd("vocabfeeds", i);
				}
			}
			//multi.exec(function(err, replies) {
				if(err){
					res.status(503);
					console.log(err);
					res.json({
						"status":"failed",
						"error":err
					});
				} else {
					res.status(200);
					res.json({
						"status":"successful",
						"response":replies
					});
				}
			});
		}
	}
});
*/

// web root
app.use('/', express.static(__dirname + '/websrc/build/default/'));

// start the server.
var server = app.listen(process.env.PORT || 3000, function () {
	console.log('Web app listening on port 3000!')
});
