
var express = require('express');
var app = express();

var cookieParser = require('cookie-parser');

var bodyParser = require('body-parser');

var redis = require("redis"),
  client = redis.createClient(process.env.REDIS_URL);

var async = require('async');

//var request = require('./api/restrequest');

const url = require('url');

app.use(cookieParser());
app.use(bodyParser.json({limit: '1mb'}));
app.use(bodyParser.urlencoded({ extended: true })); // for parsing application/x-www-form-urlencoded
app.use(bodyParser.raw({type:'image/jpeg;base64',limit: '5mb'}));
app.use(bodyParser.raw({type:'image/jpeg',limit: '5mb'}));

/*
 * Endoint and control for user login.
 * Accepts email and password and if present attempt login.
 */
app.post('/addDocument', function(req, res, next) {
  var queryData = url.parse(req.url, true).query;
  if (queryData.test) {
  	console.log(queryData.test);
	res.status(200);
	res.json({"status":"ok"});
  }
  if (req.body){
  	console.log(req.body);
  }
  return;
});

// start the server. heroku choses it's own ports, || 3000 is for local only.
var server = app.listen(process.env.PORT || 3000, function () {
  console.log('Web app listening on port 3000!')
});
