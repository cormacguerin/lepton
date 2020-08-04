
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
      var promises = [];
      const promisePush = async function() {
        for (var i=0; i<d.length; i++) {
          promises.push(new Promise((pr, pe) => {
            var database = d[i].database;
            const r = /^[0-9]+_/gi;
            db_pg['admin'].getTables(u, database, function(e,s) {
              if (e) {
                console.log(e);
                pe(e);
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
                for (var j in m) {
                  if (m[j].column_name === s_.column_name) {
                    if (m[j].fts ===  true) {
                      s_.fts = true;
                    } else {
                      s_.fts = false;
                    }
                    s_.display_field = m[j].display_field;
                    break;
                  }
                }
                if (s[i].data_type === "character varying") {
                  if (s[i].character_maximum_length === 2) {
                    s_.data_type = "varchar_2"
                  } else if (s[i].character_maximum_length === 8) {
                    s_.data_type = "varchar_8"
                  } else if (s[i].character_maximum_length === 64) {
                    s_.data_type = "varchar_64"
                  } else if (s[i].character_maximum_length === 256) {
                    s_.data_type = "varchar_256"
                  } else if (s[i].character_maximum_length === 2048) {
                    s_.data_type = "varchar_2048"
                  } else if (s[i].character_maximum_length === 8192) {
                    s_.data_type = "varchar_8192"
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
              c(response);
            }
          });
        });
      }
    });
  });
}

exports.getIndexTables = function(u,c) {
  db_pg['admin'].getTextTables(u, function(e, r) {
    if (e) {
      console.log("unable to retrieve user_clients");
      console.log(e);
      c(e);
    } else {
      const re = /^[0-9]+_/gi;
      var reply = []
      r.forEach(function(d) {
        initDB(d.database, function() {
          db_pg[d.database].getTableIndexStats(d.table, function(e,s) {
//            var d = Object.assign({},r[x]);
            d.total = parseInt(s[0].total);
            d.indexed = parseInt(s[0].indexed);
            d.refreshed = parseInt(s[0].stale);
            d.database = d.database.replace(re,'');
            reply.push(d);
            if (reply.length === r.length) {
              c(reply);
            }
          });
        });
      });
    }
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
        // add hidden search ngram tables (we do not register these so not visible by end user)
        initDB(db, function() {
          db_pg[db].addNgramTables(function(e,r) {
            if (e) {
              c({status:'failed'})
            } else {
              c({status:'success'})
            }
          })
        })
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
  initDB(db, function() {
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
    db_pg[db].runQuery(q,function(err,r) {
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

exports.updateTableColumn = function(u,d,t,c,ec,dt,edt,callback) {
  if (!(d&&t&&c&&ec&&dt&&edt)) {
    return callback({status:'failed'})
  }
  const db = u + '_' + d;
  initDB(db, function() {
    var status;
    var promises = [];
    const promisePush = async function() {
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
 * checks for table existence, 
 * d must be the full table name including owner suffix
 */
exports.checkTableExists = function(d,t,callback) {
  db_pg['admin'].checkTableExists(d,t, function(e,r) {
    if (e) {
      console.log(e);
      callback(false)
    } else {
      callback(r[0].exists)
    }
  });
}

/*
 * Updates the table where we track which table/column
 */
exports.setFTS = function(u,d,t,c,b,callback) {
  if (!(d&&t&&c&&b)) {
    return callback({status:'failed'})
  }
  const db = u + '_' + d;
  initDB('admin', function() {
    db_pg['admin'].setFTS(u, db, t, c, b, function(err, r) {
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
exports.addServingColumn = function(u,d,t,c,callback) {
  if (!(d&&t&&c)) {
    return callback({status:'failed'})
  }
  const db = u + '_' + d;
  initDB('admin', function() {
    db_pg['admin'].addServingColumn(u, db, t, c, function(err, r) {
      if (err) {
        console.log("unable to add serving column")
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
exports.removeServingColumn = function(u,d,t,c,callback) {
  if (!(d&&t&&c)) {
    return callback({status:'failed'})
  }
  const db = u + '_' + d;
  initDB('admin', function() {
    db_pg['admin'].removeServingColumn(u, db, t, c, function(err, r) {
      if (err) {
        console.log("unable to remove serving column")
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
 * enable / disable serving
 */
exports.setServing = function(u,d,t,s,callback) {
  if (!(d&&t&&s)) {
    return callback({status:'failed'})
  }
  const db = u + '_' + d;
  initDB('admin', function() {
    db_pg['admin'].setServing(u, db, t, s, function(err, r) {
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
  const db = u + '_' + d;
  initDB('admin', function() {
    db_pg['admin'].setFTSDisplayField(u, db, t, df, function(err, r) {
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

exports.addModel = function(u,l,m,c) {
	db_pg['admin'].addModel(u,l,m, function(e,r) {
		if (e){
			console.log("unable to retrieve user_clients");
			console.log(e);
            c({status:'failed',error:e})
        } else {
          c({status:'success'})
        }
	});
}

exports.saveModel = function(u,i,l,m,p,d,c) {
	db_pg['admin'].saveModel(u, i, l, m, p, d , function(e,r) {
		if (e){
			console.log("unable to retrieve user_clients");
			console.log(e);
            c({status:'failed',error:e})
        } else {
          c({status:'success'})
        }
	});
}

exports.getModels = function(u,c) {
	db_pg['admin'].getModels(u, function(e,r) {
		if (e){
			console.log("unable to retrieve user_clients");
			console.log(e);
            c([])
        } else {
          c(r)
        }
	});
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

exports.addTableData = function(db,t,data,callback) {
  if (!(db&&t&&data)) {
    return callback({status:'failed'})
  }
  if (db.length > 63) {
    return callback({status:'failed'})
  }
  initDB(db, function() {
    if (Array.isArray(data)) {
      console.log('array data');
      if (typeof t !== 'string') {
        return callback('no table provided');
      }
      db_pg[db].addTableData(t,data,function(err,r) {
        if (err) {
          console.log(err);
          callback(err)
        } else {
          callback(null,r)
        }
      });
    } else if (typeof data === 'object') {
      // TODO : we need to remove or do some work on this. maybe a separate endpoint
      console.log('object data');
      var results = [];
      var errors = [];
      // Object.keys(data).forEach(function(table) {
        db_pg[db].addTableData(t,[data],function(err,r) {
          if (err){
            console.log(err);
            errors.push(err);
          } else {
            results.push(r);
          }
        });
      // });
      callback(errors,results)
    }
  });
}

exports.deleteTable = function(u, d, t, ty, c) {
  initDB('admin', function() {
    // delete any associeted text columns
    const db = u + '_' + d;
    db_pg['admin'].deleteTextTable(u, db, t, function(err,r) {
      if (err){
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
                  if (r.length === 0) {
                    c({status:'success'})
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

/*
 * get an API key
 */
exports.addApiKey = function(user_id,n,k,callback) {
  if (!(n&&k)) {
    return callback({status:'failed'})
  }
  initDB('admin', function() {
    db_pg['admin'].addApiKey(user_id,n,k,function(err,r) {
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
 * add api scope
 */
exports.addApiScope = function(user_id,k,a,d,t,callback) {
  if (!(k&&a&&d)) {
    return callback({status:'failed'})
  }
  const db = user_id + '_' + d;
  initDB('admin', function() {
    // get a list of all the users databases and tables for validation
    db_pg['admin'].getTablesByUserId(user_id, function(e_,r) {
      // format the keys and scope in a json object
      var sources = {}
      for (var i in r) {
        if (!sources[r[i].database]) {
          sources[r[i].database] = {}
          sources[r[i].database].tables = []
        }
        sources[r[i].database].tables.push(r[i].tablename)
      }
      console.log('Object.keys(sources)')
      console.log(Object.keys(sources))
      console.log('db')
      console.log(db)
      // now we have a list lets check our database
      if (Object.keys(sources).includes(db)) {
        // if there is a table scope check that
        if (t) {
          console.log('sources')
          console.log(sources)
          console.log('sources[db]')
          console.log(sources[db])
          if (!(sources[db].tables.includes(t))) {
            return callback({status:'failed', message:'invalid table scope'})
          }
        }
        db_pg['admin'].addApiScope(k,a,db,t,user_id,function(err,r) {
          if (err) {
            console.log(err);
            callback({status:'failed', error:err, message:err.message})
          } else {
            callback({status:'success', message:sources})
          }
        });
      } else {
        callback({status:'failed', message:'invalid database scope'})
      }
    });
  });
}

/*
 * get api keys for a user
 */
exports.getApiKeys = function(user_id,callback) {
  initDB('admin', function() {
    db_pg['admin'].getApiKeys(user_id,function(err,r) {
      if (err) {
        console.log(err);
        callback({status:'failed', error:err, message:err.message})
      } else {
        // format the keys and scope in a json object
        console.log(r)
        const re = /^[0-9]+_/gi;
        var keys = {}
        for (var i in r) {
          if (!keys[r[i].name]) {
            keys[r[i].name] = {}
            keys[r[i].name].scope = []
          }
          if (r[i].api && r[i].database) {
            var scope = {}
            scope.api = r[i].api
            scope.database = r[i].database.replace(re,'')
            scope.table = r[i].table
            keys[r[i].name].scope.push(scope)
          }
          keys[r[i].name].id = r[i].id
          keys[r[i].name].key = r[i].concat
          keys[r[i].name].name = r[i].name
        }
        // convert to array for use with vue table
        var arr = []
        for (var j in keys) {
          arr.push(keys[j])
        }
        console.log('keys')
        console.log(keys)
        callback({status:'success', message:arr})
      }
    });
  });
}

/*
 * delete api key
 */
exports.deleteApiKey = function(user_id,k,callback) {
  if (!k) {
    return callback({status:'failed'})
  }
  initDB('admin', function() {
    db_pg['admin'].deleteApiKey(user_id, k, function(e,r) {
      if (e) {
        console.log(e)
        callback({status:'failed',error:e});
      } else {
        callback({status:'success',message:r});
      }
    })
  })
}

/*
 * delete api scope
 */
exports.deleteApiScope = function(user_id,k,a,d,t,callback) {
  if (!(k&&a&&d)) {
    return callback({status:'failed'})
  }
  const db = user_id + '_' + d;
  initDB('admin', function() {
    // get the keys to make sure this user is the owner (TODO need to have reader perms here also probably)
    db_pg['admin'].getApiKeys(user_id,function(err,r) {
      var isOwner = false
      for (var i in r) {
        console.log('user_id : ' + user_id)
        console.log(r[i])
        if (r[i].owner === user_id) {
          isOwner = true
        }
      }
      if (isOwner === true) {
        db_pg['admin'].deleteApiScope(k,a,db,t, function(e,r) {
          if (e) {
            console.log(e)
            callback({status:'failed',error:e});
          } else {
            callback({status:'success',message:r});
          }
        })
      } else {
        callback({status:'failed',error:'permission denied'})
      }
    })
  })
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

