
var express = require('express');
var app = express();

var cookieParser = require('cookie-parser');

var bodyParser = require('body-parser');

var redis = require("redis"),
	client = redis.createClient(process.env.REDIS_URL);

var async = require('async');

//var request = require('./api/restrequest');

const url = require('url');

const { Pool, Client } = require('pg')

const pool = new Pool({
	user: 'clio',
	host: 'localhost',
	database: 'index',
	password: 'secret',
	port: 5432,
})

pool.query('SELECT NOW()', (err, res) => {
	console.log(err, res)
	pool.end()
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
		console.log(req.body);
		if (req.body){
			var docs = req.body;
			multi = client.multi();
			for (var i in docs) {
				if (docs.hasOwnProperty(i)) {
					switch (docs[i].crawl_language) {
						case "en":
							console.log("CRAWL EN");
			  				multi.hset("doc_feed_en", i, JSON.stringify(docs[i]));
							multi.sadd("doc_id_en", i);
						case "ja":
							console.log("CRAWL JA");
			  				multi.hset("doc_feed_ja", i, JSON.stringify(docs[i]));
							multi.sadd("doc_id_ja", i);
					}

				}
			}
			multi.exec(function(err, replies) {
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

/*
 * A function to add a document(s) to our corpus.
 */
app.post('/addVocabulary', function(req, res, next) {
	var queryData = url.parse(req.url, true).query;
	if (queryData.type == "content") {
		if (req.body){
			var docs = req.body;
			multi = client.multi();
			for (var i in docs) {
				if (docs.hasOwnProperty(i)) {
	  				multi.hset("vocab_feed", i, JSON.stringify(docs[i]));
					multi.sadd("vocabfeeds", i);
				}
			}
			multi.exec(function(err, replies) {
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

// start the server. heroku choses it's own ports, || 3000 is for local only.
var server = app.listen(process.env.PORT || 3000, function () {
	console.log('Web app listening on port 3000!')
});
