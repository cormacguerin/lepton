
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

exports.getDatabases = function(u,c) {
  var res = res;
	db_pg['admin'].getDatabases(u,function(err,d) {
		if (err){
			console.log("unable to retrieve user_clients");
			console.log(err);
      c(err)
		} else {
      console.log('d');
      console.log(d);

      var promises = [];
      const promisePush = async function() {
        for (var i=0; i<d.length; i++) {
          promises.push(new Promise((pr, pe) => {
            var database = d[i].database;
            db_pg['admin'].getTables(database, function(e,s) {
              if (e) {
                console.log(e);
                c(e);
              } else {
                pr({key:database,tables:s});
              }
            });
          }));
        }
        await Promise.all(promises)
        .then((r)=> {
          console.log('getDatabases response');
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

/*
exports.getTables = function(database, c) {
  console.log("database");
  console.log(database);
  initDB(database, function() {
    db_pg[database].getTables(function(e, t) {
      if (err){
        console.log("unable to retrieve user_clients");
        console.log(err);
        c(e);
      } else {
        console.log('getTables response');
        console.log(t);
            } else {
            }
          });
        }
        c(s);
      }
    });
  });
}
*/

exports.getTableSchema = function(database, table, c) {
  console.log('getTableSchema for database ' + database + ' table ' + table);
  initDB(database, function() {
    db_pg[database].getTableSchema(database, table, function(e, s) {
      if (e) {
        console.log("unable to retrieve user_clients");
        console.log(e);
        c(e);
      } else {
        var response = [] 
        for (var i in s) {
          var s_ = {};
          s_.column_name = s[i].column_name; 
          if (s[i].data_type === "character varying") {
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
        console.log('response');
        console.log(response);
        c(response);
      }
    });
  });
}

exports.addDatabase = function(u,d,c) {
  if (!d) {
    c({status:'failed'})
  }
  if (d.length > 63) {
    c({status:'failed'})
  }
	db_pg['admin'].addDatabase(u, d, function(e,r) {
		if (e){
			console.log("unable to retrieve user_clients");
			console.log(e);
      c({status:'failed',error:e})
		} else {
      if (r.length === 0) {
        c({status:'success'})
      } else {
        c({status:'failed'})
      }
    }
	});
}

exports.createTable = function(u,d,t,c,dt,callback) {
  if (!d) {
    callback({status:'failed'})
  }
  if (d.length > 63) {
    callback({status:'failed'})
  }
  initDB(d, function() {
    db_pg[d].createTable(u,d,t,c,dt,function(err,r) {
      if (err){
        console.log("unable to create table");
        console.log(err);
        callback({status:'failed',error:err})
      } else {
        if (r.length === 0) {
          db_pg['admin'].registerTable(u,d,t,'data',null, function(e,r) {
            if (e) {
              console.log(e);
              callback({status:'failed'})
            } else {
              callback({status:'success'})
            }
          });
        } else {
          callback({status:'failed'})
        }
      }
    });
  });
}

exports.createSearchTable = function(u,d,t,callback) {
  if (!d) {
    callback({status:'failed'})
  }
  if (d.length > 63) {
    callback({status:'failed'})
  }
  initDB(d, function() {
    db_pg[d].createSearchTable(u,d,t,function(err,r) {
      if (err){
        console.log("unable to create search table");
        console.log(err);
        callback({status:'failed',error:err})
      } else {
        console.log(r);
        console.log('r.length');
        console.log(r.length);
        if (r.length === 0) {
          db_pg['admin'].registerTable(u,d,t,'search',null, function(e,r) {
            if (e) {
              console.log(e);
              callback({status:'failed'})
            } else {
              callback({status:'success'})
            }
          });
        } else {
          callback({status:'failed'})
        }
      }
    });
  });
}

exports.createDataSetTable = function(u,d,t,q,callback) {
  if (!d) {
    callback({status:'failed'})
  }
  if (d.length > 63) {
    callback({status:'failed'})
  }
  initDB(d, function() {
    db_pg[d].runQuery(q,function(err,r) {
      if (err){
        console.log("unable to create search table");
        console.log(err);
        callback({status:'failed',error:err})
      } else {
        console.log(r);
        console.log('r.length');
        console.log(r.length);
        if (r.length > 0) {
          var dataset = {}
          dataset.query = q;
          dataset.fields = Object.keys(r[0]);
          console.log('dataset');
          console.log(dataset);
          db_pg['admin'].registerTable(u,d,t,'dataset',dataset, function(e,r) {
            if (e) {
              console.log(e);
              callback({status:'failed'})
            } else {
              callback({status:'success'})
            }
          });
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

exports.deleteTable = function(d, t, c) {
  initDB(d, function() {
    db_pg[d].deleteTable(d, t, function(err,r) {
      if (err) {
        console.log("unable to retrieve user_clients");
        console.log(err);
        c({status:'failed',error:err})
      } else {
        console.log(r);
        console.log('r.length');
        console.log(r.length);
        if (r.length === 0) {
          db_pg['admin'].unregisterTable(d,t, function(e,r) {
            if (e) {
              console.log(e);
              c({status:'failed'})
            } else {
              c({status:'success'})
            }
          });
        } else {
          c({status:'failed'})
        }
      }
    });
  });
}

exports.deleteDatabase = function(d,c) {
  if (db_pg[d]) {
	  db_pg[d].end();
    delete db_pg[d];
  }
	db_pg['admin'].deleteDatabase(d, function(err,r) {
		if (err) {
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

exports.runQuery = function(d,q,callback) {
  if (!(d&&q)) {
    return callback({status:'failed'})
  }
  initDB(d, function() {
    db_pg[d].runQuery(q, function(err,r) {
      if (err){
        console.log(err);
        callback({status:'failed', error:err})
      } else {
        callback({status:'success', message:r})
      }
    });
  });
}

function initDB(database, callback) {

  var fs = require('fs')

  var password;

  function get_last_line(filename, callback) {
    var data = fs.readFileSync(filename, 'utf8');
    var lines = data.split("\n");

    if(lines.length===0){
      callback('empty file');
    }
    callback(null, lines[lines.length-2]);
  }

  get_last_line('./dbpassword', function(err, line){
    if (err) {
      console.log(err);
      console.log('no dbpassword file found? usually created when creating the database with server_files/setup_db.sh');
      callback();
    } else {
      password = line;
    }
  });

  console.log(password);

  if (!db_pg[database]) {
    db_pg[database] = new pg({database,password})
  }
  callback();
}

