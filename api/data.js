
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
            const r = /^[0-9]+_/gi;
            db_pg['admin'].getTables(u, database, function(e,s) {
              if (e) {
                console.log(e);
                c(e);
              } else {
                pr({key:database.replace(r,''),tables:s});
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

exports.getTableSchema = function(user_id, d, table, c) {
  const database = user_id + '_' + d;
  console.log('getTableSchema for database ' + database + ' table ' + table);
  initDB('admin', function() {
    db_pg['admin'].getTableMeta(user_id, database, table, function(e, m) {
      if (e) {
        callback(e);
      } else {
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
                console.log('m');
                console.log(m);
                console.log('s_.column_name')
                console.log(s_.column_name)
                for (var j in m) {
                  if (m[j].column_name === s_.column_name) {
                    console.log('m[j]')
                    console.log(m[j])
                    s_.fts = true;
                    s_.display_field = m[j].display_field;
                    break;
                  }
                }
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
  const db = u + '_' + d;
	db_pg['admin'].addDatabase(u, db, function(e,r) {
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
  const db = u + '_' + d;
  initDB(db, function() {
    db_pg[db].createTable(db,t,c,dt,function(err,r) {
      if (err){
        console.log("unable to create table");
        console.log(err);
        callback({status:'failed',error:err})
      } else {
        if (r.length === 0) {
          db_pg['admin'].registerTable(u,db,t,'data',null, function(e,r) {
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
  const db = u + '_' + d;
  initDB(d, function() {
    db_pg[db].createSearchTable(db,t,function(err,r) {
      if (err){
        console.log("unable to create search table");
        console.log(err);
        callback({status:'failed',error:err})
      } else {
        console.log(r);
        console.log('r.length');
        console.log(r.length);
        if (r.length === 0) {
          db_pg['admin'].registerTable(u,db,t,'search',null, function(e,r) {
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
  const db = u + '_' + d;
  initDB(db, function() {
    db_pg[db].runQuery(u,q,function(err,r) {
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
          db_pg['admin'].registerTable(u,db,t,'dataset',dataset, function(e,r) {
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

exports.deleteDataSetTable = function(u,d,t,callback) {
  if (!d) {
    callback({status:'failed'})
  }
  if (d.length > 63) {
    callback({status:'failed'})
  }
  const db = u + '_' + d;
  initDB('admin', function() {
    db_pg['admin'].unregisterTable(u,db,t,'dataset', function(e,r) {
      if (e) {
        console.log(e);
        callback({status:'failed'})
      } else {
        callback({status:'success'})
      }
    });
  });
}

exports.addTableColumn = function(u,d,t,c,dt,callback) {
  if (!(d&&t&&c)) {
    return callback({status:'failed'})
  }
  const db = u + '_' + d;
  initDB(db, function() {
    db_pg[db].addTableColumn(t, c, dt, function(err,r) {
      if (err){
        console.log("unable to retrieve user_clients");
        console.log(err);
        callback({status:'failed',error:err})
      } else {
        if (r.length === 0) {
            callback({status:'success',message:r})
        } else {
          callback({status:'failed'})
        }
      }
    });
  });
}

exports.updateTableColumn = function(u, d,t,c,ec,dt,edt,callback) {
  if (!(d&&t&&c&&ec&&dt&&edt)) {
    return callback({status:'failed'})
  }
  initDB(db, function() {
    var status;
    var promises = [];
    const promisePush = async function() {
      const db = u + '_' + d;
      var column;
      if (c !== ec) {
        db_pg[db].renameTableColumn(t, c, ec, function(err,r) {
          promises.push(new Promise((pr, pe) => {
            if (err) {
              pe(err)
            } else {
              var promises_ = promises;
              if (dt !== edt) {
                db_pg[db].setTableColumnDataType(t, c, dt, function(err,r) {
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
          db_pg[db].setTableColumnDataType(t, c, dt, function(err,r) {
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
          callback({status:'success', message:r})
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
exports.setFTS = function(u,d,t,c,b,callback) {
  if (!(d&&t&&c&&b)) {
    return callback({status:'failed'})
  }
  initDB('admin', function() {
    db_pg['admin'].setFTS(u, d, t, c, b, function(err, r) {
      if (err) {
        console.log("unable to retrieve user_clients")
        console.log(err)
        callback({status:'failed', error:err})
      } else {
        if (r.length === 0) {
          callback(r)
        } else {
          callback(err)
        }
      }
    })
  })
}

/*
 * Updates the table where we track which table/column
 */
exports.setFTSDisplayField = function(u,d,t,df,callback) {
  if (!(d&&t&&df)) {
    return callback({status:'failed'})
  }
  initDB('admin', function() {
    db_pg['admin'].setFTSDisplayField(u, d, t, df, function(err, r) {
      if (err) {
        console.log("unable to retrieve user_clients")
        console.log(err)
        callback({status:'failed', error:err})
      } else {
        console.log(r)
        console.log('r.length')
        console.log(r.length)
        if (r.length === 0) {
          callback(r)
        } else {
          callback(err)
        }
      }
    })
  })
}

exports.deleteTableColumn = function(u,d,t,c,callback) {
  if (!(d&&t&&c)) {
    return callback({status:'failed'})
  }
  initDB('admin', function() {
    db_pg['admin'].deleteTextColumn(u, d, t, c, function(err, r) {
      if (err ) {
        console.log("unable to delete text column");
        console.log(err);
        callback({status:'failed', error:err})
      } else {
        const db = u + '_' + d;
        initDB(db, function() {
          db_pg[db].deleteTableColumn(t, c, function(err,r) {
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
    });
  });
}

exports.addTableData = function(u,d,t,data,callback) {
  if (!(d&&data)) {
    return callback({status:'failed'})
  }
  if (d.length > 63) {
    return callback({status:'failed'})
  }
  const db = u + '_' + d;
  initDB(db, function() {
    if (Array.isArray(data)) {
      if (typeof t !== 'string') {
        return callback('no table provided');
      }
      db_pg[db].addTableData(t,data,function(err,r) {
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
        db_pg[db].addTableData(table,data[table],function(err,r) {
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

exports.deleteTable = function(u, d, t, ty, c) {
  initDB('admin', function() {
    // delete any associeted text columns
    const db = u + '_' + d;
    db_pg['admin'].deleteTextTable(u, db, t, function(err,r) {
      if (err){
        console.log("unable to retrieve user_clients");
        console.log(err);
        callback({status:'failed', error:err})
      } else {
        // unregister the table
        db_pg['admin'].unregisterTable(u,db,t,ty, function(e,r) {
          if (e) {
            console.log(e);
            c({status:'failed to unregister table'})
          } else {
            initDB(db, function() {
              db_pg[db].deleteTable(t, function(err,r) {
                if (err) {
                  console.log("unable to retrieve user_clients");
                  console.log(err);
                  c({status:'failed',error:err})
                } else {
                  console.log(r);
                  console.log('r.length');
                  console.log(r.length);
                  if (r.length === 0) {
                    db_pg['admin'].unregisterTable(db,t,ty, function(e,r) {
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
        });
      }
    });
  });
}

exports.deleteDatabase = function(u,d,c) {
  const db = u + '_' + d;
  if (db_pg[db]) {
	  db_pg[db].end();
    delete db_pg[db];
  }
	db_pg['admin'].deleteDatabase(u, db, function(err,r) {
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

exports.runQuery = function(u,d,q,callback) {
  if (!(d&&q)) {
    return callback({status:'failed'})
  }
  const db = u + '_' + d;
  initDB(db, function() {
    db_pg[db].runQuery(q, function(err,r) {
      if (err){
        console.log(err);
        callback({status:'failed', error:err, message:err.message})
      } else {
        callback({status:'success', message:r})
      }
    });
  });
}

exports.addChart = function(user_id,d,ds,n,c,cd,callback) {
  if (!(d&&ds&&n&&c)) {
    return callback({status:'failed'})
  }
  const db = user_id + '_' + d;
  initDB('admin', function() {
    db_pg['admin'].addChart(user_id,db,ds,n,c,cd, function(err,r) {
      if (err){
        console.log(err);
        callback({status:'failed', error:err, message:err.message})
      } else {
        callback({status:'success', message:r})
      }
    });
  });
}

/*
 * get chart
 */
exports.getMyCharts = function(id,callback) {
  if (!(d&&q)) {
    return callback({status:'failed'})
  }
  initDB('admin', function() {
    db_pg['admin'].getChartsByOwner(id, function(err,r) {
      if (err){
        console.log(err);
        callback({status:'failed', error:err, message:err.message})
      } else {
        callback({status:'success', message:r})
      }
    });
  });
}

/*
 * get chart
 */
exports.getChartById = function(id,callback) {
  if (!(d&&q)) {
    return callback({status:'failed'})
  }
  initDB('admin', function() {
    db_pg['admin'].getChartById(id, function(err,r) {
      if (err){
        console.log(err);
        callback({status:'failed', error:err, message:err.message})
      } else {
        callback({status:'success', message:r})
      }
    });
  });
}

/*
 * get chart
 */
exports.getChartById = function(id,callback) {
  if (!(d&&q)) {
    return callback({status:'failed'})
  }
  initDB('admin', function() {
    db_pg['admin'].getChartById(id, function(err,r) {
      if (err){
        console.log(err);
        callback({status:'failed', error:err, message:err.message})
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

