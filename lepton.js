
var express = require('express');
var app = express();

var cookieParser = require('cookie-parser');

var bodyParser = require('body-parser');

var redis = require("redis"),
	client = redis.createClient(process.env.REDIS_URL);

var async = require('async');

//var request = require('./api/restrequest');

const url = require('url');

const { Pool } = require('pg')

const pool = new Pool({
	user: 'postgres',
	host: 'localhost',
	database: 'index',
	password: 'VE1XhwRsmGf5e+8VFEvTS8fNBNqQZE93hNjVINAu2Aw=',
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
		console.log(req.body);
		if (req.body){
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
                                          console.log(docs[i]);
                                                const client = await pool.connect()
                                                try {
			  			        //multi.hset("doc_feed_ja", i, JSON.stringify(docs[i]));
						        //multi.sadd("doc_id_ja", i);
                                                        const reply = 
                                                            await client.query("INSERT INTO docs(url, feed, lang, crawl_date)"
                                                                + " VALUES("
                                                                + "\'" + i + "\',"
                                                                + "\'" + JSON.stringify(docs[i]) + "\'," 
                                                                + "\'" + docs[i].crawl_language + "\',"
                                                                + "NOW()"
                                                                + ");");
                                                        console.log("reply " + reply.rows[0]);
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

// start the server. heroku choses it's own ports, || 3000 is for local only.
var server = app.listen(process.env.PORT || 3000, function () {
	console.log('Web app listening on port 3000!')
});
