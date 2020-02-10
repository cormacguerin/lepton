
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
        var response = [] 
        for (var i in s) {
          var s_ = {};
          s_.column_name = s[i].column_name; 
          if (s[i].data_type === "character varying") {
            console.log('deb 1');
            console.log('s[i]');
            console.log(s[i]);
            if (s[i].character_maximum_length === 64) {
              s_.data_type = "varchar_64"
            } else if (s[i].character_maximum_length === 256) {
              s_.data_type = "varchar_256"
            } else if (s[i].character_maximum_length === 2048) {
              s_.data_type = "varchar_2048"
            } else {
              s_.data_type = null
            }
          } else if (s[i].data_type === "numeric(16,2)") {
            s_.data_type = "decimal"
          } else if (s[i].data_type === "numeric(32,8)") {
            s_.data_type = "bigdecimal"
          } else {
            s_.data_type = s[i].data_type;
          }
          response.push(s_);
        }
        console.log(response);
        c(response);
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
      c({status:'failed',error:err})
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
        callback({status:'failed',error:err})
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

exports.addTableColumn = function(d,t,c,dt,df,fts,callback) {
  if (!(d&&t&&c)) {
    return callback({status:'failed'})
  }
  if (fts==='true'&&(!df||df==='undefined'||df===null)) {
    return callback({status:'failed'})
  }
  initDB(d, function() {
    db_pg[d].addTableColumn(t, c, dt, function(err,r) {
      if (err){
        console.log("unable to retrieve user_clients");
        console.log(err);
        callback({status:'failed',error:err})
      } else {
        if (r.length === 0) {
          if (dt === 'text') {
            setFTS(d,t,c,df,fts, function(e,r) {
              if (e) {
                callback({status:'failed', message:'column updated successfully, but unable to enable full text search', error:e});
              } else {
                callback({status:'success',message:r})
              }
            });
          } else {
            callback({status:'success',message:r})
          }
        } else {
          callback({status:'failed'})
        }
      }
    });
  });
}

exports.updateTableColumn = function(d,t,c,ec,dt,edt,df,fts,callback) {
  if (!(d&&t&&c&&ec&&dt&&edt)) {
    return callback({status:'failed'})
  }
  if (fts==='true'&&(!df||df==='undefined'||df===null)) {
    return callback({status:'failed'})
  }
  console.log("dt " + dt)
  console.log("df " + df)
  console.log("fts " + fts)
  initDB(d, function() {
    var status;
    var promises = [];
    const promisePush = async function() {
      var column;
      if (c !== ec) {
        db_pg[d].renameTableColumn(t, c, ec, function(err,r) {
          promises.push(new Promise((pr, pe) => {
            if (err) {
              pe(err)
            } else {
              var promises_ = promises;
              if (dt !== edt) {
                db_pg[d].setTableColumnDataType(t, c, dt, function(err,r) {
                  promises_.push(new Promise((pr, pe) => {
                    if (err) {
                      pe(err)
                    } else {
                      pr(r)
                    }
                  }));
                });
              }
              pr(r)
            }
          }));
        });
      } else {
        if (dt !== edt) {
          db_pg[d].setTableColumnDataType(t, c, dt, function(err,r) {
            promises.push(new Promise((pr, pe) => {
              if (err) {
                pe(err)
              } else {
                pr(r)
              }
            }));
          });
        }
      }
      await Promise.all(promises)
      .then((r)=> {
        console.log(r);
        if (dt === 'text') {
          setFTS(d,t,c,df,fts, function(e,r) {
            if (e) {
              callback({status:'failed', message:'column updated successfully, but unable to set full text search', error:e});
            } else {
              callback({status:'success', message:r})
            }
          });
        } else {
          callback({status:'success', message:r})
        }
      })
      .catch((e) => {
        console.log(e);
        callback({status:'failed',error:e})
      });
    }
    promisePush();
  });
}

/*
 * Updates the table where we track which table/column
 */
function setFTS(d,t,c,df,b,callback) {
  if (!(d&&t&&c&&b)) {
    return callback({status:'failed'})
  }
  initDB('admin', function() {
    db_pg['admin'].setFTS(d, t, c, df, b, function(err, r) {
      if (err) {
        console.log("unable to retrieve user_clients")
        console.log(err)
        callback({status:'failed', error:err})
      } else {
        console.log(r)
        console.log('r.length')
        console.log(r.length)
        if (r.length === 0) {
          callback(null,r)
        } else {
          callback(err)
        }
      }
    })
  })
}

exports.deleteTableColumn = function(d,t,c,callback) {
  if (!(d&&t&&c)) {
    return callback({status:'failed'})
  }
  initDB(d, function() {
    db_pg[d].deleteTableColumn(t, c, function(err,r) {
      if (err){
        console.log("unable to retrieve user_clients");
        console.log(err);
        callback({status:'failed', error:err})
      } else {
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
    if (Array.isArray(data)) {
      if (typeof t !== 'string') {
        return callback('no table provided');
      }
      db_pg[d].addTableData(t,data,function(err,r) {
        if (err) {
          console.log("unable to retrieve user_clients");
          console.log(err);
          callback({r,error:err})
        } else {
          callback({r})
        }
      });
    } else if (typeof data === 'object') {
      var results = [];
      Object.keys(data).forEach(function(table) {
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
      c({status:'failed',error:err})
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

