
// admin console postgres instance
const pg = require('./postgres.js');

// database postgres instances placeholder
var db_pg = {}
db_pg['admin'] = new pg({database:'admin'});

var async = require('async');

const url = require('url');

var aesgcm = require('../aesgcm256');

var jwt = require('jsonwebtoken');

var secret = new Buffer('secret', 'base64');

var userClients = {};

exports.clients = userClients;

exports.getDatabases = function(c) {
  var res = res;
	db_pg['admin'].getDatabases(function(err,d) {
		if (err){
			console.log("unable to retrieve user_clients");
			console.log(err);
      c(err)
		} else {

      var promises = [];
      const promisePush = async function() {

        for (i=0; i<d.length; i++) {
          initDB(d[i].datname, function() {
            promises.push(new Promise((pr, pe) => {
              var database = d[i].datname;
              db_pg[database].getTables(function(err, s) {
                if (err) {
                  console.log("unable to retrieve user_clients");
                  console.log(err);
                  pr(err);
                } else {
                  pr({key:database,tables:s});
                }
              });
            }));
          });
        }

        await Promise.all(promises)
        .then((r)=> {
          console.log(r);
          c(r);
        })
        .catch((e) => {
          console.log(e);
          c(e);
        });
      }
      promisePush();
    }
	});
}

exports.getTables = function(database, c) {
  console.log("database");
  console.log(database);
  initDB(database, function() {
    db_pg[database].getTables(function(err, s) {
      if (err){
        console.log("unable to retrieve user_clients");
        console.log(err);
        c(e);
      } else {
        console.log(s);
        c(s);
      }
    });
  });
}

exports.getTableSchema = function(database, table, c) {
  console.log("database");
  console.log(database);
  initDB(database, function() {
    db_pg[database].getTableSchema(database, table, function(err, s) {
      if (err) {
        console.log("unable to retrieve user_clients");
        console.log(err);
        c(e);
      } else {
        console.log(s);
        c(s);
      }
    });
  });
}

exports.addDatabase = function(d,c) {
  if (!d) {
    c({status:'failed'})
  }
  if (d.length > 63) {
    c({status:'failed'})
  }
	db_pg['admin'].addDatabase(d, function(err,r) {
		if (err){
			console.log("unable to retrieve user_clients");
			console.log(err);
      c({status:'failed',message:err})
		} else {
      console.log(r);
      console.log('r.length');
      console.log(r.length);
      if (r.length === 0) {
        c({status:'success'})
      } else {
        c({status:'failed'})
      }
    }
	});
}

exports.createTable = function(d,t,c,dt,callback) {
  if (!d) {
    callback({status:'failed'})
  }
  if (d.length > 63) {
    callback({status:'failed'})
  }
  initDB(d, function() {
    db_pg[d].createTable(t, c, dt, function(err,r) {
      if (err){
        console.log("unable to retrieve user_clients");
        console.log(err);
        callback({status:'failed',message:err})
      } else {
        console.log(r);
        console.log('r.length');
        console.log(r.length);
        if (r.length === 0) {
          callback({status:'success'})
        } else {
          callback({status:'failed'})
        }
      }
    });
  });
}

exports.addTable = function(d,t,c,dt,callback) {
  if (!(d&&t&&c)) {
    return callback({status:'failed'})
  }
  initDB(d, function() {
    db_pg[d].addTable(t, c, dt, function(err,r) {
      if (err){
        console.log("unable to retrieve user_clients");
        console.log(err);
        callback({status:'failed',message:err})
      } else {
        console.log(r);
        console.log('r.length');
        console.log(r.length);
        if (r.length === 0) {
          callback({status:'success'})
        } else {
          callback({status:'failed'})
        }
      }
    });
  });
}

exports.addTableData = function(d,t,data,callback) {
  if (!(d&&data)) {
    return callback({status:'failed'})
  }
  if (d.length > 63) {
    return callback({status:'failed'})
  }
  initDB(d, function() {
    console.log("d " + d);
    console.log("t " + t);
    if (Array.isArray(data)) {
      if (typeof t !== 'string') {
        return callback('no table provided');
      }
      db_pg[d].addTableData(t,data,function(err,r) {
        if (err) {
          console.log("unable to retrieve user_clients");
          console.log(err);
          callback({r,message:err})
        } else {
          callback({r})
        }
      });
    } else if (typeof data === 'object') {
      var results = [];
      Object.keys(data).forEach(function(table) {
        console.log("table " +table);
        db_pg[d].addTableData(table,data[table],function(err,r) {
          if (err){
            console.log("unable to retrieve user_clients");
            console.log(err);
            results.push(err);
          } else {
            results.push(r);
          }
        });
      });
      callback(results)
    }
  });
}

exports.deleteDatabase = function(d,c) {
  if (db_pg[d]) {
	  db_pg[d].end();
    delete db_pg[d];
  }
	db_pg['admin'].deleteDatabase(d, function(err,r) {
		if (err) {
			console.log("unable to retrieve user_clients");
			console.log(err);
      c({status:'failed',message:err})
		} else {
      console.log(r);
      console.log('r.length');
      console.log(r.length);
      if (r.length === 0) {
        c({status:'success'})
      } else {
        c({status:'failed'})
      }
    }
  });
}

function initDB(database, callback) {
  if (!db_pg[database]) {
    db_pg[database] = new pg({database,password:"kPwFWfYAsyRGZ6IomXLCypWqbmyAbK+gnKIW437QLjw="})
  }
  callback();
}

