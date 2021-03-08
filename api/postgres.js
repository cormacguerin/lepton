/*
 * Wrapper for Postgres.
 */

const { Pool } = require('pg');
var fs = require('fs')

class Postgres {

  constructor(db) {

    if (db) {

      console.log('db')
      console.log(db)

      this.pool = new Pool({
        user: db.postgres_user,
        host: db.postgres_host,
        database: db.postgres_database,
        password: db.postgres_password,
        port: 5432,
      });

      this.pool.on('error', (err, client) => {
        console.error('Unexpected error on idle client', err)
        process.exit(-1)
      })

      this.pool.query('SELECT NOW()', (err, res) => {
        console.log(err, res)
        // this.pool.end()
      })

      this.logger = fs.createWriteStream('postgres_write.log', {
        flags: 'a' // 'a' means appending (old data will be preserved)
      })
    }
  }

  execute(statement, values, callback) {
    console.log(statement);
    console.log(values);
    (async () => {
      const client = await this.pool.connect()
      try {
        const reply = await client.query(statement, values)
        this.logger.write(statement + "\n");
        callback(null, reply.rows)
      } finally {
        client.release()
      }
    })().catch(
      e => {
        console.log(e.stack);
        callback(e);
      }
    )
  }

  /*
   * same as above, but in row mode array
   */
  raexecute(statement, values, callback) {
    console.log(statement);
    console.log(values);
    const query = {
      text: statement,
      values: values,
      rowMode: 'array',
    };
    (async () => {
      const client = await this.pool.connect()
      try {
        const reply = await client.query(query)
        this.logger.write(statement + "\n");
        callback(null, reply.rows)
      } finally {
        client.release()
      }
    })().catch(
      e => {
        console.log(e.stack);
        callback(e);
      }
    )
  }

  batch_execute(statement, values, keys, callback) {
    console.log(statement);
    (async () => {
      this.logger.write(statement + "\n");
      const client = await this.pool.connect();
      try {
        await client.query("BEGIN");
        var promises = [];
        const promisePush = async function() {
var it = new Date().getTime();
          for (const v in values) {
            var k = [...keys];
            for (var x in values[v]) {
              // set empty stuff to null
              if (values[v][x] === "") {
                values[v][x] = null;
              }
              var i = k.indexOf(x);
              if (i > -1) {
                k.splice(i, 1);
              }
            }
            // we may be missing some values so check and insert null if not.
            if (k.length > 0) {
              for (var j in k) {
                values[v][k[j]] = null;
              }
              // at this point the order of the object might be messed up (note we should probably use arrays instead.)
              var tmp_obj = {}
              for (var x in keys) {
                  tmp_obj[keys[x]] = values[v][keys[x]];
              }
              values[v] = tmp_obj;
            }
            //
            /*         
            console.log('v');
            console.log(v);
            console.log('values[v]');
            console.log(values[v]);
            console.log('Object.values(values[v])');
            console.log(Object.values(values[v]));
            console.log(' - - - - ');
            */
            console.log(values[v])
            promises.push(client.query(statement, Object.values(values[v])));
          }
          var et = new Date().getTime();
          var totaltime = et-it;
          console.log("promises pushed in " + totaltime + "ms");
          var it = new Date().getTime();
          await Promise.all(promises)
          .then((r)=> {
            console.log("primises done, commit");
            client.query("COMMIT");
            client.release();
            var et = new Date().getTime();
            var totaltime = et-it;
            console.log("promises finished in " + totaltime + "ms");
            callback(null, r);
          })
          .catch((e) => {
            console.log('error pushing statements')
            console.log(e);
            client.query("ROLLBACK");
             client.release();
            callback(e);
          });
        }
        promisePush();
      } catch(e) {
        console.log('error in rollback');
        console.log(e);
        await client.query("ROLLBACK");
        client.release();
      } finally {
        console.log("finally");
        // client.release();
      }
    })().catch(
      e => {
        console.log(e.stack);
        callback(e);
      }
    )
  }

  end() {
    this.pool.end();
  }

  // apparenlty using a token like $edivy$ either side means you don't need to do any crazy escaping.
  format(text) {
    if (text) {
      return '$edivy$' + text.replace("$","\$") + '$edivy$';
    } else {
      return null;
    }
  }

  /*
   * initialize database from schema
   */
  init(database, callback) {
    var query = fs.readFileSync('./server/admin_schema.psql').toString();

    var vm = this;
    this.execute(query, null, function(e,r) {
      if (e) {
        console.log(e)
        callback();
      } else {
        callback(r)
      }
    });
  }

  /*
   * Add database
   * We prepend database names with the user_id to ensure no duplicates.
   * Separately we tracks user databases in a database table.
   */
  addDatabase(user, database, callback) {

    var query = "CREATE DATABASE \""
      + database
      + "\" WITH owner postgres"
      + " ENCODING 'UTF8'";
    //  + " LC_COLLATE = 'en_US.UTF-8'"
    //  + " LC_CTYPE = 'en_US.UTF-8';"

    var vm = this;
    this.execute(query, null, function(e,r) {
      if (e) {
        callback(e, r);
      } else {
        var query = "INSERT INTO databases(database,owner,reader,writer) VALUES($1,$2,$2,$2);"
        vm.execute(query, [database, user], function(e,r) {
            callback(e, r);
        });
      }
    });
  }

  addNgramTables(callback) {
    var vm = this;
    var promises = [];
    const promisePush = async function() {
      var tables = ['unigrams','bigrams','trigrams']
      for (var t in tables) {
        promises.push(new Promise((pr, pe) => {
          var query = "CREATE TABLE "
            + tables[t]
            + " (id SERIAL PRIMARY KEY,"
            + " gram VARCHAR(256) NOT NULL UNIQUE,"
            + " lang VARCHAR(2) NOT NULL,"
            + " idf real);"
          vm.execute(query, null, function(e,r) {
            if (e) {
              pe(e)
            } else {
              pr(r)
            }
          });
        }));
      }
      await Promise.all(promises)
      .then((r)=> {
        callback(null, r);
      })
      .catch((e) => {
        console.log('error pushing statements')
        callback(e)
      })
    }
    promisePush();
  }

  getDatabases(user, callback) {
    // var query = "SELECT datname FROM pg_database WHERE datistemplate = false;"
    var query = "SELECT database FROM databases WHERE reader = $1;"
    this.execute(query, [user], function(e,r) {
      console.log(e);
      console.log(r);
      callback(e,r);
    });
  }

  /*
   * Create database
   * We also add a unique internal uuid to each table
   * This can uniquely identify an item and also be used as a shard key
   */
  createTable(database, table, column, datatype, callback) {

    var query = "CREATE TABLE \""
      + table
      + "\" (\""
      + column
      + "\""
      + getDataType(datatype)
      + " PRIMARY KEY,"
      + "feed_date TIMESTAMP,"
      + "uuid uuid);"

    this.execute(query, null, function(e,r) {
      callback(e,r);
    });
  }

  /*
   * Create search table
   * we assume documents are text
   * TODO we also wanna be able to upload other tpyes of docs, word etc
   * in this case I suggest customer sends json as document say with structure like this
   * {
   *  data: <data here>
   *  encoding: base64
   * }
   * internally we scan , is first char '{'
   * if so can we json parse it?
   * does it contain data and encoding?
   * if yes decode and index
   * if no index as normal text
   */
  createSearchTable(database, table, callback) {

    var query = "CREATE TABLE \""
      + table
      + "\" ("
      + "language VARCHAR(2),"
      + "url VARCHAR(2048) PRIMARY KEY,"
      + "title VARCHAR(2048),"
      + "status VARCHAR(64),"
      + "last_modified TIMESTAMP,"
      + "document text,"
      + "metadata jsonb,"
      + "id SERIAL UNIQUE,"
      + "uuid uuid,"
      + "docscore real,"
      + "tdscore real,"
      + "entities text,"
      + "atf real,"
      + "feed_date TIMESTAMP,"
      + "index_date TIMESTAMP,"
      + "update BOOL,"
      + "raw_text text,"
      + "segmented_grams jsonb);"

    /*
      + "feed jsonb,"
      + "raw_text text[],"
      + "unigrams text[],"
      + "bigrams text[],"
      + "trigrams text[],"
      + "unigram_positions text[],"
      + "bigram_positions text[],"
      + "trigram_positions text[],"
    */

    this.execute(query, null, function(e,r) {
      if (r) {
        var i_dir = "index/"
        if (!fs.existsSync(i_dir)){
          fs.mkdirSync(i_dir);
        }
        var id_dir = "index/" + database.replace(/ /g,"_");
        if (!fs.existsSync(id_dir)){
          fs.mkdirSync(id_dir);
        }
        var idt_dir = "index/" + database.replace(/ /g,"_") + "/" + table.replace(/ /g,"_");
        if (!fs.existsSync(idt_dir)){
          fs.mkdirSync(idt_dir);
        }
      }
      callback(e,r);
    });
  }

  /*
   * After adding a table above, if successful we register it here.
   */
  registerTable(user, database, table, type, data, callback) {
    var query = "INSERT INTO tables(database,tablename,type,owner,data) VALUES((SELECT id FROM databases WHERE database = $1),$2,$3,$4,$5) ON CONFLICT ON CONSTRAINT tables_database_tablename_owner_key DO UPDATE SET database=(SELECT id FROM databases WHERE database = $1), type=$3, owner=$4, data=$5 WHERE tables.tablename=$2;"
    this.execute(query, [database, table, type, user, data], function(e,r) {
        callback(e, r);
    });
  }

  /*
   * 
   */
  unregisterTable(user, database, table, type, callback) {
    /*
    var query = "DELETE FROM tables WHERE tablename = \'"
      + table
      + "\'"
      + " AND database = \'"
      + database
      + "\' AND type = \'"
      + type
      + "\';"
      */
    var query = "DELETE FROM tables WHERE tablename = $1 AND database = (SELECT id FROM databases WHERE database = $2) AND type = $3 AND OWNER = $4;"
    this.execute(query, [table, database, type, user], function(e,r) {
        callback(e, r);
    });
  }

  getTable(user, database, tablename, callback) {
    var query = "SELECT tablename, type, data FROM tables WHERE database = (SELECT id FROM databases WHERE database = $1) and tablename = $2 AND owner = $3;"
    this.execute(query, [database, tablename, user], function(e,r) {
      callback(e,r);
    });
  }

  getTables(user, database, callback) {
    var query = "SELECT tablename, type, data FROM tables WHERE database = (SELECT id FROM databases WHERE database = $1) AND owner = $2;"
    this.execute(query, [database, user], function(e,r) {
      callback(e,r);
    });
  }

  getTableSchema(database, table, callback) {
    var query = "SELECT column_name, data_type, character_maximum_length FROM INFORMATION_SCHEMA.COLUMNS WHERE udt_catalog=$1 AND table_name = $2;"
    this.execute(query, [database,table], function(e,r) {
      callback(e,r);
    });
  }

  getTablesByUserId(user_id, callback) {
    var query = "SELECT databases.database, tablename FROM tables INNER JOIN databases ON databases.id = tables.database WHERE databases.owner = $1 ORDER BY database;"
    this.execute(query, [user_id], function(e,r) {
      callback(e,r);
    });
  }

  getTableMeta(user_id, database, table, callback) {
    var query = "SELECT _column AS column_name, display_field, serving, indexing as fts from text_tables_index WHERE _table = (SELECT id FROM tables WHERE tablename = $2 AND database = (SELECT id FROM databases WHERE database = $1) AND owner = $3)"
    this.execute(query, [database,table,user_id], function(e,r) {
      callback(e,r);
    });
  }

  getTextTables(user_id, callback) {
    var query = "SELECT d.database, t.tablename AS table, array_agg(tx._column ORDER BY t.tablename) AS column, display_field, indexing, serving from text_tables_index tx INNER JOIN databases d on d.id = tx.database INNER JOIN tables t on tx._table = t.id WHERE d.owner = $1 AND indexing = true GROUP BY d.database, t.tablename, display_field, indexing, serving ORDER BY d.database;"
    this.execute(query, [user_id], function(e,r) {
      callback(e,r);
    });
  }

  getServingTables(user_id, callback) {
    var query = "SELECT d.database, t.tablename AS table, (SELECT (array_agg(_column))) AS column FROM text_tables_index tx INNER JOIN databases d on d.id = tx.database INNER JOIN tables t on tx._table = t.id WHERE d.owner = $1 AND tx._column is not null GROUP BY d.database, t.tablename;"
    // var query = "SELECT d.database, t.tablename AS table, (SELECT array_agg(tx._column ORDER BY t.tablename) WHERE serving = true) AS column, serving from text_tables_index tx INNER JOIN databases d on d.id = tx.database INNER JOIN tables t on tx._table = t.id WHERE d.owner = $1 AND serving IS NOT NULL GROUP BY d.database, t.tablename, serving ORDER BY d.database;"
    this.execute(query, [user_id], function(e,r) {
      callback(e,r);
    });
  }

  getTableIndexStats(table, callback) {
    var query = "SELECT COUNT(*) AS total, sum(case when index_date IS NOT NULL then 1 else 0 end) AS indexed, sum(case when index_date < feed_date then 1 else 0 end) AS stale FROM \"" + table+ "\"";
    this.execute(query, null, function(e,r) {
      callback(e,r);
    });
  }

  checkTableExists(database, table, callback) {
    var query = "SELECT EXISTS (SELECT FROM tables WHERE database = (SELECT id FROM databases WHERE database = $1) AND tablename = $2)"
    this.execute(query, [database,table], function(e,r) {
      callback(e,r);
    });
  }

  /*
   * Add database
   */
  addTableColumn(table, column, datatype, callback) {

    var query = "ALTER TABLE \""
      + table 
      + "\" ADD COLUMN \""
      + column
      + "\" "
      + getDataType(datatype)
      + ";"

    this.execute(query, null, function(e,r) {
      callback(e, r);
    });
  }

  /*
   * Rename
   */
  renameTableColumn(table, column, old_column, callback) {

    var query = "ALTER TABLE \""
      + table
      + "\" RENAME COLUMN \""
      + old_column
      + "\" TO \""
      + column
      + "\";"

    this.execute(query, null, function(e,r) {
      callback(e, r);
    });
  }

  /*
   * Rename
   */
  setTableColumnDataType(table, column, datatype, callback) {

    var query = "ALTER TABLE \""
      + table
      + "\" ALTER COLUMN \""
      + column
      + "\" TYPE "
      + getDataType(datatype)
      + ";"

    this.execute(query, null, function(e,r) {
      callback(e, r);
    });
  }

  /*
   * Add database
   */
  deleteTableColumn(table, column, callback) {

    var query = "ALTER TABLE \""
      + table
      + "\" DROP COLUMN \""
      + column
      + "\"";

    this.execute(query, null, function(e,r) {
      callback(e, r);
    });
  }

  deleteTextColumn(user_id, database, table, column, callback) {
    var query = "DELETE FROM text_tables_index WHERE _column = $3 AND database = (SELECT id FROM databases WHERE database = $1 AND owner = $4) AND _table = (SELECT id FROM tables WHERE tablename = $2)"
    this.execute(query, [database, table, column, user_id], function(e,r) {
      callback(e, r);
    });
  }

  /*
   * Delete text tables
   */
  deleteTextTable(user_id, database, table, callback) {
    var query = "DELETE FROM text_tables_index WHERE database = (SELECT id FROM databases WHERE database = $1 AND owner = $3) AND _table = (SELECT id FROM tables WHERE tablename = $2)"
    this.execute(query, [database, table, user_id], function(e,r) {
      callback(e, r);
    });
  }

  addTableData(table, data, callback) {
    var it = new Date().getTime();
    var pkey = "SELECT constraint_name FROM information_schema.table_constraints WHERE table_name = $1 AND constraint_type = 'PRIMARY KEY'"
    var this_ = this;
    this.execute(pkey, [table], function(e,r) {
      if (e) {
        console.log(e);
        return callback(e);
      }
      var primary_key;
      if (r[0]) {
        primary_key = r[0].constraint_name;
      } else {
        return callback('unknown primary key, please provide a primary key.');
      }
      // console.log('primary_key : ' + primary_key);
      var result = {
        status:'',
        errors: [],
        results: []
      }
      // set the current feed_date
      for (var i=0; i<data.length; i++) {
        data[i]['feed_date'] = "NOW()";
      }

      var keys = Object.keys(data[0]);
      var insert_prep = '';
      for (var i=1; i<=keys.length; i++) {
        insert_prep += '$' + i.toString();
        if (i < keys.length) {
          insert_prep += ', ';
        }
      }
      var values_prep = '';
      for (var i=1; i<=keys.length; i++) {
        values_prep += '\"';
        values_prep += keys[i-1];
        values_prep += '\" = ';
        values_prep += '$' + i.toString();
        if (i < keys.length) {
          values_prep += ', ';
        }
      }
      // console.log('insert_prep : ' + insert_prep);
      // console.log('values_prep : ' + values_prep);
      var quotedKeys = "\"" + keys.join("\",\"") + "\"";
      var statement = "INSERT INTO \""
        + table
        + "\" ("
        + quotedKeys
        + ") VALUES("
        + insert_prep
        + ")"
        + " ON CONFLICT ON CONSTRAINT \""
        + primary_key
        + "\" DO UPDATE SET "
        + values_prep 
        + ";"

      console.log(statement);
      console.log(data);
      this_.batch_execute(statement, data, keys, function(e,r) {
        var this__ = this_;
        if (e) {
          console.log(e);
          result.errors.push(e);
          if (result.status === 'success' || result.status === 'succeeded with errors') {
            result.status = 'succeeded with errors';
          } else {
            result.status = 'failed';
          }
        } else {
          // makes huge responses and becomes very slow
          // result.results.push(r);
          if (result.status === 'failed' || result.status === 'succeeded with errors') {
            result.status = 'succeeded with errors';
          } else {
            result.status = 'success';
          }
        }
        var et = new Date().getTime();
        var totaltime = et-it;
        console.log("total time taken for " + table + " is " + totaltime + "ms");
        return callback(e, result);
      });
    });
  }

  /*
   * Delete table
   */
  deleteTable(table, callback) {

    var query = "DROP TABLE \""
      + table
      + "\";"

    this.execute(query, null, function(e,r) {
      callback(e, r);
    });
  }

  /*
   * Delete database
   */
  deleteDatabase(user_id, database, callback) {

    var disable = "UPDATE pg_database SET datallowconn = 'false' WHERE datname = $1;"
    var disconnect = "SELECT pg_terminate_backend(pid) FROM pg_stat_activity WHERE datname = $1;"
    var drop = "DROP DATABASE \""
      + database
      + "\";"

    var this_ = this;
    this.execute(disable, [database], function(e,r) {
      if (e) {
        console.log(e);
        callback(e);
      } else {
        this_.execute(disconnect, [database], function(e,r) {
          var this__ = this_;
          if (e) {
            console.log(e);
                callback(e, r);
          } else {
            this__.execute(drop, null, function(e,r) {
              var query = "DELETE FROM text_tables_index WHERE database = (SELECT id FROM databases where database = $1 AND owner = $2)"
              var this___ = this__;
              this__.execute(query, [database, user_id], function(e,r) {
                if (e) {
                  console.log(e);
                  callback(e, r);
                } else {
                  this__.execute(drop, null, function(e,r) {
                    var query = "DELETE FROM databases WHERE database = $1 AND owner = $2"
                    this___.execute(query, [database, user_id], function(e,r) {
                      callback(e, r);
                    });
                  });
                }
              });
            });
          }
        });
      }
    });
  }

  /*
   * Add Chart
   */
  addChart(user_id, database, dataset, name, chart, chartdata, callback) {

    var query = "INSERT INTO charts(name,database,dataset,chart,data,owner) VALUES($1,(SELECT id FROM databases WHERE database = $2),(SELECT id FROM tables WHERE tablename = $3),$4,$5,$6) ON CONFLICT ON CONSTRAINT charts_name_owner_key DO UPDATE SET database=(SELECT id FROM databases WHERE database = $2), dataset=(SELECT id FROM tables WHERE tablename = $3), chart=$4, data=$5 WHERE charts.name=$1 and charts.owner=$6;"

    console.log(query);

    this.execute(query, [name, database, dataset, chart, chartdata, user_id], function(e,r) {
      callback(e, r);
    });
  }

  /*
   * get chart
   */
  getChartsByOwner(id, callback) {
    var query = "SELECT database, dataset, chart, chartdata FROM charts where owner = $1;"
    this.execute(query, [id], function(e,r) {
      callback(e,r);
    });
  }

  /*
   * get chart
   */
  getChartById(id, callback) {
    var query = "SELECT database, dataset, chart, chartdata FROM charts where id = $1;"
    this.execute(query, [id], function(e,r) {
      callback(e,r);
    });
  }

  /*
   * User Functions
   */
  addUser(username, email, password, default_view, facebook_user_id, facebook_info, account_type, account_status, confirm_code, color_code, role, callback) {

    var query = "INSERT INTO users(username, email, password, default_view, facebook_user_id, facebook_info, account_type, account_status, confirm_code, color_code, role, created_date)"
      + " VALUES($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12) RETURNING id;"
      var values = [
        username,
        email,
        password,
        default_view,
        facebook_user_id,
        facebook_info,
        account_status,
        account_type,
        confirm_code,
        color_code,
        role,
        new Date().getTime()
      ]
    this.execute(query, values, function(e,r) {
      if (e) {
        callback(e);
      } else {
        callback(null,r[0].id);
      }
    });
  }

  confirmUser(user_id, callback) {
    var query = "UPDATE users SET account_status = 'confirmed' WHERE id = $1;"
    this.execute(query, [user_id], function(e,r) {
      callback(e,r);
    });
  }

  resetConfirmCode(email, confirm_code, callback) {
    var query = "UPDATE users SET confirm_code = $1 WHERE email = $2;"
    this.execute(query, [confirm_code, email], function(e,r) {
      if (e) {
        callback(false);
      } else {
        callback(true);
      }
    });
  }

  resetPassword(email, password, confirm_code, callback) {
    var query = "UPDATE users SET password = $1 WHERE email = $2 AND confirm_code = $3;"
    this.execute(query, [password, email, confirm_code], function(e,r) {
      if (e) {
        callback(false);
      } else {
        callback(true);
      }
    });
  }

  getUserClients(callback) {
    var query = "SELECT * FROM user_clients;"
    this.execute(query, null, function(e,r) {
      callback(e,r);
    });
  }

  checkTableForValue(table, column, value, callback) {
    var query = "SELECT COUNT("
      + column
      + ") FROM "
      + table
      + " WHERE "
      + column
      + " = $1;"
    this.execute(query, [value], function(e,r) {
      if (e) {
        console.log(e);
        callback(false);
      } else {
        if (parseInt(r[0].count) > 0) {
          callback(true);
        } else {
          callback(false);
        }
      }
    });
  }

  addUserClient(client, callback) {
    var query = "INSERT INTO user_clients (client_id, user_id, success, created_date, active_date, token, message)"
      + " VALUES($1,$2,$3,$4,$5,$6,$7)"
      + " ON CONFLICT (user_id,client_id) DO UPDATE SET"
      + " active_date = $8,"
      + " token = $9,"
      + " message = $10;"
    var values = [
      client.client_id,
      client.user_id,
      client.success,
      new Date().getTime(),
      new Date().getTime(),
      client.token,
      client.message,
      new Date().getTime(),
      client.token,
      client.message
    ]
    this.execute(query, values, function(e,r) {
      callback(e,r);
    });
  }

  removeUserClient(client, callback) {
    var query = "DELETE FROM user_clients WHERE client_id = $1 AND user_id = $2;"
    this.execute(query, [client.client_id, client.user_id], function(e,r) {
      callback(e,r);
    });
  }

  getUserInfo(key, values, callback) {
    var values_prep = '';
    for (var i=1; i<=values.length; i++) {
      values_prep += '$' + i.toString();
      if (i < values.length) {
        values_prep += ', ';
      }
    }
    var query;
    if (Number.isInteger(parseInt(key))) {
      query = "SELECT " + values.join(',') + " FROM users WHERE id = $1;"
    } else {
      query = "SELECT " + values.join(',') + " FROM users WHERE email = $1;"
    }
    this.execute(query, [key], function(e,r) {
      if (r) {
        if (r[0]) {
          callback(e,r[0]);
        } else {
          callback(e,null);
        }
      } else {
        callback(e,null);
      }
    });
  }

  getUsernameFromId(user_id, callback) {
    if (Number.isInteger(parseInt(user_id)) ) {
      var query = "SELECT username FROM users WHERE id = $1;"
      this.execute(query, [user_id], function(e,r) {
        if (r) {
          if (r[0]) {
            callback(e,r[0].username);
          } else {
            callback(e,null);
          }
        } else {
          callback(e,null);
        }
      });
    }
  }

  setFTS(u,d,t,c,b,callback) {
    var query = "INSERT INTO text_tables_index(database,_table,_column,indexing)"
      + " SELECT r.db, r.t, r.c, r.e::boolean FROM"
      + " (SELECT * FROM (VALUES ((SELECT id FROM databases WHERE database = $1 AND owner = $5), (SELECT id FROM tables where tablename = $2 AND database = (SELECT id from databases where database = $1 AND owner = $5)), $3, $4)) AS v (db,t,c,e)) r"
      + " ON CONFLICT ON CONSTRAINT text_tables_index_database__table__column_key DO UPDATE SET"
      + " _table = (SELECT id FROM tables where tablename = $2 AND database = (SELECT id from databases where database = $1 AND owner = $5)),"
      + " _column = $3,"
      + " indexing = $4::boolean";
    /*
    var query = "SELECT t.id, r.db, r.t, r.c, r.e::boolean FROM"
      + " (SELECT id FROM tables WHERE database = $1 AND tablename = $2 AND owner = $5) t,"
      + " (SELECT * FROM (VALUES ($1, $2, $3, $4)) AS v (db,t,c,e)) r"
    */
    var values = [d,t,c,b,u]

    this.execute(query, values, function(e,r) {
      console.log(e);
      console.log(r);
      callback(e,r);
    });
  }

  addServingColumn(u,d,t,c,callback) {
    var query = "INSERT INTO text_tables_index(database,_table,_column,serving) VALUES ((SELECT id FROM databases WHERE database = $1 AND owner = $4), (SELECT id FROM tables where tablename = $2 AND database = (SELECT id from databases where database = $1 AND owner = $4)), $3, true) ON CONFLICT DO NOTHING";

    var values = [d,t,c,u]

    this.execute(query, values, function(e,r) {
      console.log(e);
      console.log(r);
      callback(e,r);
    });
  }

  getServingColumns(u,d,t,callback) {
    var query = "SELECT _column FROM text_tables_index WHERE database = (SELECT id FROM databases WHERE database = $1 AND databases.owner = $3) and _table = (SELECT id FROM tables WHERE tablename = $2) AND serving = true;";

    var values = [d,t,u]

    this.execute(query, values, function(e,r) {
      console.log(e);
      console.log(r);
      callback(e,r);
    });
  }

  setServing(u,d,t,s,callback) {
    var query = "UPDATE text_tables_index SET serving = $3::boolean WHERE _table = (SELECT id FROM tables where tablename = $2 AND database = (SELECT id from databases where database = $1 AND owner = $4));"

    var values = [d,t,s,u]

    this.execute(query, values, function(e,r) {
      console.log(e);
      console.log(r);
      callback(e,r);
    });
  }

  setServingColumn(u,d,t,s,c,callback) {
    var query = "UPDATE text_tables_index SET serving = $3::boolean WHERE _table = (SELECT id FROM tables where tablename = $2 AND database = (SELECT id from databases where database = $1 AND owner = $5) AND _column = $4);"

    var values = [d,t,s,c,u]

    this.execute(query, values, function(e,r) {
      console.log(e);
      console.log(r);
      callback(e,r);
    });
  }

  setFTSDisplayField(u, d,t,df,callback) {
    var query = "UPDATE text_tables_index SET display_field = $3 WHERE id = (SELECT id FROM tables where database = $1 AND tablename = $2 AND owner = $4)"

    var values = [d,t,df,u]

    this.execute(query, values, function(e,r) {
      console.log(e);
      console.log(r);
      callback(e,r);
    });
  }

  addModel(u,l,m, callback) {
    var query = "INSERT INTO ml_models(model,language,owner) VALUES($1,$2,$3)"

    var values = [m,l,u]

    this.execute(query, values, function(e,r) {
      console.log(e);
      console.log(r);
      callback(e,r);
    });
  }

  saveModel(u,i,l,m,p,d, callback) {
    var query = "UPDATE ml_models(model,language,program,dataset) VALUES($1,$2,$3,$4) WHERE owner = $5 and id = $6"

    var values = [m,l,p,d,u,i]

    this.execute(query, values, function(e,r) {
      console.log(e);
      console.log(r);
      callback(e,r);
    });
  }

  getModels(u, callback) {
    var query = "SELECT * FROM ml_models WHERE u = $1"

    var values = [u]

    this.execute(query, values, function(e,r) {
      console.log(e);
      console.log(r);
      callback(e,r);
    });
  }

  getApiKeys(u, callback) {
    var query = "SELECT api_keys.id, owner, name, concat(LEFT(key,5),'...'), api_scopes.api, (SELECT database FROM databases WHERE id = api_scopes.database), (SELECT tablename FROM tables WHERE id = api_scopes._table) AS table FROM api_keys FULL OUTER JOIN api_scopes ON api_keys.id = api_scopes.id WHERE owner = $1;"

    var values = [u]

    this.execute(query, values, function(e,r) {
      console.log(e);
      console.log(r);
      callback(e,r);
    });
  }

  // used for API authorization
  getApiKeyById(id, callback) {
    var query = "SELECT api_keys.id, owner, name, key, api_scopes.api, (SELECT database FROM databases WHERE id = api_scopes.database), (SELECT tablename FROM tables WHERE id = api_scopes._table) AS table FROM api_keys FULL OUTER JOIN api_scopes ON api_keys.id = api_scopes.id WHERE api_keys.id = $1;"

    var values = [id]

    this.execute(query, values, function(e,r) {
      console.log(e);
      console.log(r);
      var key = {}
      key.scope = []
      const reg = /^[0-9]+_/gi;
      for (var i in r) {
        if (r[i].id && r[i].key && r[i].database) {
          key.id = r[i].id;
          key.owner = r[i].owner;
          key.name = r[i].name;
          key.key = r[i].key;
          var scope = {}
          scope.api = r[i].api;
          scope._database = r[i].database;
          scope.database = r[i].database.replace(reg,'');
          scope.table = r[i].table;    
          key.scope.push(scope);
        }
      }
      console.log('key')
      console.log(key)
      callback(e,key);
    });
  }

  addApiKey(u, n, k, callback) {
    var query = "INSERT INTO api_keys(key,name,owner) VALUES ($1,$2,$3)"

    var values = [k,n,u]

    this.execute(query, values, function(e,r) {
      console.log(e);
      console.log(r);
      callback(e,r);
    });
  }

  addApiScope(k, a, d, t, u, callback) {
    var query;
    var values;
    if (t) {
      query = "INSERT INTO api_scopes(id,api,database,_table) VALUES ($1,$2,(SELECT id from databases WHERE database = $3 AND owner = $5),(SELECT id FROM tables WHERE tablename = $4))"
      values = [k,a,d,t,u]
    } else {
      query = "INSERT INTO api_scopes(id,api,database) VALUES ($1,$2,(SELECT id from databases WHERE database = $3 AND owner = $4))"
      values = [k,a,d,u]
    }

    this.execute(query, values, function(e,r) {
      console.log(e);
      console.log(r);
      callback(e,r);
    });
  }

  deleteApiKey(u, k, callback) {
    var query = "WITH del AS (DELETE FROM api_keys WHERE id = $1 AND owner = $2 RETURNING id) DELETE FROM api_scopes WHERE id = (SELECT id FROM del)"

    var values = [k,u]

    this.execute(query, values, function(e,r) {
      console.log(e);
      console.log(r);
      callback(e,r);
    });
  }

  deleteApiScope(k, a, d, t, callback) {
    var query;
    var values;
    if (t) {
      query = "DELETE FROM api_scopes WHERE id = $1 AND api = $2 AND database = (SELECT id FROM databases WHERE database = $3) AND _table = (SELECT id from tables WHERE tablename = $4)"
      var values = [k,a,d,t]
    } else {
      query = "DELETE FROM api_scopes WHERE id = $1 AND api = $2 AND database = (SELECT id FROM databases WHERE database = $3)"
      var values = [k,a,d]
    }

    this.execute(query, values, function(e,r) {
      console.log(e);
      console.log(r);
      callback(e,r);
    });
  }

  runQuery(q,callback) {
    console.log(q);

    this.execute(q, null, function(e,r) {
      console.log(e);
      console.log(r);
      callback(e,r);
    });
  }

  /* - - - - - - - - -*/

  getUserIdFromFacebookId(facebook_user_id, callback) {
    var query = "SELECT id FROM users WHERE facebook_user_id = '" + facebook_user_id + "';"
    this.execute(query, function(e,r) {
      if (r) {
        if (r[0]) {
          callback(e,r[0].id);
        } else {
          callback(e,null);
        }
      } else {
        callback(e,null);
      }
    });
  }

  getUsernamesFromIds(list, callback) {
    var listString;
    if (list) {
      if (list.length > 1) {
        listString = list.join('\',\'');
      } else {
        listString = list;
      }
    } else {
      callback();
    }
    if (!listString.length === 0) {
      callback(null,[]);
    }
    var query = "SELECT id, username FROM users WHERE id IN (\'" + listString + "\');"
    this.execute(query, function(e,r) {
      callback(e,r);
    });
  }

  /*
   * param 1 is the email to id. returns email address.
   * param 2 is the subject userid. returns username.
   */
  getMailUserInfo(to_id, sender_id, callback) {
    var query = "SELECT id, username, email FROM users WHERE id IN (" + to_id + "," + sender_id + ") ORDER BY ID ASC;"
    this.execute(query, function(e,r) {
      var reply = {};
      if (e) {
        return callback(e);
      } else if (r[0].id===to_id) {
        reply.email=r[0].email;
        reply.username=r[1].username;
      } else if (r[1].id===to_id) {
        reply.email=r[1].email;
        reply.username=r[0].username;
      } else {
        e="invalid user/email lookup";
      }
      callback(e, reply);
    });
  }

  setUserMailIfNone(user_id, email, callback) {
    var query = "UPDATE users set email = \'" + email + "\' WHERE id = \'" + user_id + "\' AND email ~ '^\\d+@edivy.com' AND \'" + email + "\' NOT IN (SELECT email FROM users);"
    this.execute(query, function(e,r) {
      callback(e,r);
    });
  }

  setUserMail(user_id, email, callback) {
    var query = "UPDATE users set email = \'" + email + "\' WHERE id = \'" + user_id + "\' AND \'" + email + "\' NOT IN (SELECT email FROM users);"
    this.execute(query, function(e,r) {
      callback(e,r);
    });
  }

  addToMailQueue(email, content, callback) {
    var query = "INSERT INTO mail_queue (email, mail_content, status) "
      + " VALUES("
      + "\'" + email + "\',"
      + "\'" + content + "\',"
      + "\'new\');"
    this.execute(query, function(e,r) {
      callback(e,r);
    });
  }

  getMailQueueLength(callback) {
    var query = "SELECT COUNT(email) FROM mail_queue WHERE STATUS = 'new';"
    this.execute(query, function(e,r) {
      if (r) {
        if (r[0]) {
          callback(e,r[0].count);
        } else {
          callback(e,0);
        }
      } else {
        callback(e,0);
      }
    });
  }

  getFromMailQueue(callback) {
    var query = "SELECT id, mail_content FROM mail_queue WHERE status = 'new' LIMIT 1;"
    this.execute(query, function(e,r) {
      if (r) {
        if (r[0]) {
          callback(e,r[0]);
        } else {
          callback(e,null);
        }
      } else {
        callback(e,null);
      }
    });
  }

  setMailQueueStatus(id, status, callback) {
    var query = "UPDATE mail_queue SET status = \'" + status + "\' where id =  \'" + id + "\';"
    this.execute(query, function(e,r) {
      if (r) {
        if (r[0]) {
          callback(e,r[0]);
        } else {
          callback(e,null);
        }
      } else {
        callback(e,null);
      }
    });
  }

  /*
   * Events functions
   */


  /*
   * This function gets a schedule. eg, the main details, and the list of events associated with it.
   */
  getSchedule(user_id, schedule_key, callback) {
    var query = "SELECT * FROM events WHERE user_id=\'" + user_id + "\' AND schedule_key = \'" + schedule_key + "\' ORDER BY start_date ASC;"
    this.execute(query, function(e,r) {
      if (r) {
        if (r[0]) {
          var schedule = {};
          schedule.commencement_date = r[0].commencement_date;
          schedule.course = r[0].course;
          schedule.timezone_offset = r[0].timezone_offset;
          schedule.schedule_key = r[0].schedule_key;
          schedule.events = [];
          for (i in r) {
            var item = {"start_date":r[i].start_date,"end_date":r[i].end_date,"event_key":r[i].event_key}
            schedule.events.push(item);
          }
          callback(e,schedule);
        } else {
          callback(e,null);
        }
      } else {
        callback(e,null);
      }
    });
  }

  /*
   * This function gets all the events associated with a schedule.
   */
  getScheduleEvents(user_id, schedule_key, callback) {
    var query = "SELECT * FROM events WHERE user_id=\'" + user_id + "\' AND schedule_key = \'" + schedule_key + "\' ORDER BY start_date ASC;"
    this.execute(query, function(e,r) {
      if (r) {
        if (r[0]) {
          var schedule = [];
          for (i in r) {
            var event = {};
            event.commencement_date = r[i].commencement_date;
            event.event_num = r[i].event_num;
            event.class_id = r[i].class_id;
            event.event_status = r[i].event_status;
            event.course = r[i].course;
            event.timezone_offset = r[i].timezone_offset;
            event.schedule_key = r[i].schedule_key;
            event.event_key = r[i].event_key;
            event.start_date = r[i].start_date;
            event.end_date = r[i].end_date;
            schedule.push(event);
          }
          callback(e,schedule);
        } else {
          callback(e,null);
        }
      } else {
        callback(e,null);
      }
    });
  }

  getEvent(user_id, event_key, callback) {
    var query = "SELECT * FROM events WHERE user_id=\'" + user_id + "\' AND event_key = \'" + event_key + "\';"
    this.execute(query, function(e,r) {
      if (r) {
        if (r[0]) {
          var event = {};
          event.start_date = r[0].start_date;
          event.end_date = r[0].end_date;
          event.commencement_date = r[0].commencement_date;
          event.course = r[0].course;
          event.timezone_offset = r[0].timezone_offset;
          event.schedule_key = r[0].schedule_key;
          event.bookings = r[0].bookings;
          event.event_key = r[0].event_key;
          event.event_num = r[0].event_num;
          event.class_id = r[0].class_id;
          event.status = r[0].status;
          callback(e,event);
        } else {
          callback(e,null);
        }
      } else {
        callback(e,null);
      }
    });
  }

  getEventList(user_id, event_keys, callback) {
    var listString;
    if (event_keys) {
      if (event_keys.length > 1) {
        listString = event_keys.join('\',\'');
      } else {
        listString = event_keys;
      }
    } else {
      callback();
    }
    if (!listString.length === 0) {
      callback(null,[]);
    }
    var query = "SELECT * FROM events WHERE user_id=\'" + user_id + "\' AND event_key IN (\'" + listString + "\');"
    this.execute(query, function(e,r) {
      if (r) {
        if (r.length > 0) {
          var events = [];
          for (i in r) {
            var event = {};
            event.start_date = r[i].start_date;
            event.end_date = r[i].end_date;
            event.commencement_date = r[i].commencement_date;
            event.course = r[i].course;
            event.timezone_offset = r[i].timezone_offset;
            event.schedule_key = r[i].schedule_key;
            event.event_key = r[i].event_key;
            event.event_num = r[i].event_num;
            event.class_id = r[i].class_id;
            event.status = r[i].status;
            events.push(event);
          }
          callback(e,events);
        } else {
          callback(e,null);
        }
      } else {
        callback(e,null);
      }
    });
  }

  getBookingEventList(user_id, booking_id, callback) {
    var query = "SELECT * FROM events WHERE user_id=\'" + user_id + "\' AND bookings @> ARRAY[\'" + booking_id + "\']::int[]";
    this.execute(query, function(e,r) {
      if (r) {
        if (r.length > 0) {
          var events = [];
          for (i in r) {
            var event = {};
            event.start_date = r[i].start_date;
            event.end_date = r[i].end_date;
            event.commencement_date = r[i].commencement_date;
            event.course = r[i].course;
            event.timezone_offset = r[i].timezone_offset;
            event.schedule_key = r[i].schedule_key;
            event.event_key = r[i].event_key;
            event.event_num = r[i].event_num;
            event.class_id = r[i].class_id;
            event.status = r[i].status;
            events.push(event);
          }
          callback(e,events);
        } else {
          callback(e,null);
        }
      } else {
        callback(e,null);
      }
    });
  }

  /*
   * Get the events for a given user.
   */
  getUserEvents(user_id, callback) {
    var query = "SELECT * FROM events WHERE user_id=\'" + user_id + "\' ORDER BY start_date ASC;"
    this.execute(query, function(e,r) {
      if (r) {
        if (r.length > 0) {
          var events = [];
          for (i in r) {
            var event = {};
            event.start_date = r[i].start_date;
            event.end_date = r[i].end_date;
            event.commencement_date = r[i].commencement_date;
            event.course = r[i].course;
            event.timezone_offset = r[i].timezone_offset;
            event.schedule_key = r[i].schedule_key;
            event.event_key = r[i].event_key;
            event.event_num = r[i].event_num;
            event.class_id = r[i].class_id;
            event.bookings = r[i].bookings;
            event.status = r[i].status;
            events.push(event);
          }
          callback(e,events);
        } else {
          callback(e,null);
        }
      } else {
        callback(e,null);
      }
    });
  }

  /*
   * Get the standalone events for a given user.
   * If an optional class_id is supplied we only are getting events for a prospective user booking.
   * In this case return events that are..
   * empty OR have that class booked AND are not already booked for said student AND if the student is also a teacher does not have a slot at the same time
   */
  getStandaloneEvents(user_id, teacher_id, class_id, callback) {
    if ((parseInt(teacher_id) === parseInt(user_id)) && parseInt(class_id)) {
      // if here it's an authenticated request for a teachers own class
      var query = "SELECT * FROM events WHERE course = FALSE AND user_id=\'" + teacher_id + "\' ORDER BY start_date ASC;"
    } else if (parseInt(user_id) && parseInt(teacher_id) && parseInt(class_id)) {
      // if here it's an authenticated request for a class
      var query = "SELECT * FROM events AS ev WHERE course = FALSE AND user_id=\'" + teacher_id + "\' AND (class_id=\'" + class_id + "\' OR class_id IS NULL) AND ev.event_key NOT IN (SELECT event_key FROM events WHERE user_id=\'" + user_id + "\') AND 0 = (SELECT count(bev.event_key) FROM events AS bev WHERE bev.event_key IN (SELECT unnest(b.events) FROM bookings AS b WHERE b.student_id=\'" + user_id + "\') AND (ev.start_date BETWEEN bev.start_date AND bev.end_date OR ev.end_date BETWEEN bev.start_date AND bev.end_date)) ORDER BY start_date ASC;"
    } else if (!user_id && parseInt(teacher_id) && parseInt(class_id)) {
      // if here it's an un-authenticated request for a class
      var query = "SELECT * FROM events WHERE course = FALSE AND user_id=\'" + teacher_id + "\' AND (class_id=\'" + class_id + "\' OR class_id IS NULL) ORDER BY start_date ASC;"
    } else {
      return callback("unknown request");
    }
    this.execute(query, function(e,r) {
      if (r) {
        if (r.length > 0) {
          var events = [];
          for (i in r) {
            var event = {};
            event.start_date = r[i].start_date;
            event.end_date = r[i].end_date;
            event.commencement_date = r[i].commencement_date;
            event.course = r[i].course;
            event.timezone_offset = r[i].timezone_offset;
            event.schedule_key = r[i].schedule_key;
            event.event_key = r[i].event_key;
            event.event_num = r[i].event_num;
            event.class_id = r[i].class_id;
            event.status = r[i].status;
            events.push(event);
          }
          callback(e,events);
        } else {
          callback(e,null);
        }
      } else {
        callback(e,null);
      }
    });
  }

  /*
   * Test if there is a standalone event(from a list of events) that is already booked for a different class.
   * This is important so that no one books a standalone class that is already booked for a different class.
   * Course events are also processed here but would always return false since they are hardcoded to a class.
   */
  haveStandaloneEventsClassBookings(user_id, class_id, event_keys, callback) {
    var listString;
    if (event_keys) {
      if (event_keys.length > 1) {
        listString = event_keys.join('\',\'');
      } else {
        listString = event_keys;
      }
    } else {
      callback();
    }
    if (!listString.length === 0) {
      callback(false);
    }
    var query = "SELECT COUNT(id) FROM events WHERE user_id=\'" + user_id + "\' AND class_id!=\'" + class_id + "\' AND status='booked' AND course=false AND event_key IN (\'" + listString + "\');"
    this.execute(query, function(e,r) {
      if (e) {
        console.log(e);
        callback(false);
      } else {
        if (parseInt(r[0].count) > 0) {
          callback(true);
        } else {
          callback(false);
        }
      }
    });
  }

  /*
   * Has a schedule active bookings bookings? (to ensure we don't delte or remove a class that is active)
   */
  hasScheduleActiveBookings(user_id, schedule_key, callback) {
    var query = "SELECT COUNT(id) FROM bookings WHERE request_status != 'completed' AND request_status != 'failed' AND request_status != 'feedback' AND id IN (SELECT unnest(bookings) FROM events WHERE teacher_id = \'" + user_id + "\' AND schedule_key = \'" + schedule_key + "\');"
    this.execute(query, function(e,r) {
      if (e) {
        console.log(e);
        callback(false);
      } else {
        if (parseInt(r[0].count) > 0) {
          callback(true);
        } else {
          callback(false);
        }
      }
    });
  }

  /*
   * Has a schedule active bookings bookings? (to ensure we don't delte or remove a class that is active)
   */
  hasEventActiveBookings(user_id, event_key, callback) {
    var query = "SELECT COUNT(id) FROM bookings WHERE request_status != 'completed' AND id IN (SELECT unnest(bookings) FROM events WHERE teacher_id = \'" + user_id + "\' AND event_key = \'" + event_key + "\');"
    this.execute(query, function(e,r) {
      if (e) {
        console.log(e);
        callback(false);
      } else {
        if (parseInt(r[0].count) > 0) {
          callback(true);
        } else {
          callback(false);
        }
      }
    });
  }

  /*
   * Has existing events (used when creating a schedule to ensure a teacher does not double list times)
   */
  hasExistingEvents(user_id, events, callback) {
    var start_and_end = "";
    for (var i=0; i<events.length;i++) {
      var and_or = "";
      if (i===0) {
        and_or = " AND (";
      } else {
        and_or = " OR ";
      }
      start_and_end += and_or + "(start_date BETWEEN "
        + new Date(events[i].start_date).getTime() + " AND "
        + new Date(events[i].end_date).getTime() + " OR end_date BETWEEN "
        + new Date(events[i].start_date).getTime() + " AND "
        + new Date(events[i].end_date).getTime() + ")";
    }
    var query = "SELECT COUNT(id) FROM events WHERE user_id = \'" + user_id + "\'" + start_and_end + ");" 
    this.execute(query, function(e,r) {
      if (e) {
        console.log(e);
        callback(false);
      } else {
        if (parseInt(r[0].count) > 0) {
          callback(true);
        } else {
          callback(false);
        }
      }
    });
  }

  /*
   * we add every booking ID into the bookgs array, of course some of these may be rejected/cancelled but we can query against them to retrieve only accepted ones if we need.
   */

  /*
   * we add every booking ID into the bookgs array, of course some of these may be rejected/cancelled but we can query against them to retrieve only accepted ones if we need.
   */
  setRequestClassEvent(user_id, event_key, class_id, booking_id, callback) {
    var query = "UPDATE events SET bookings = array_append(bookings, \'" + booking_id + "\'), status = 'requested', class_id = \'" + class_id + "\' WHERE user_id = \'" + user_id + "\' AND event_key = \'" + event_key + "\';"
    this.execute(query, function(e,r) {
      callback(e,r);
    });
  }

  /*
   * Update the class ID as we have accepted this.
   */
  setBookClassEvent(user_id, event_key, class_id, booking_id, callback) {
    var query = "UPDATE events SET status='booked', class_id = \'" + class_id + "\' WHERE user_id = \'" + user_id + "\' AND event_key = \'" + event_key + "\';"
    this.execute(query, function(e,r) {
      callback(e,r);
    });
  }

  /*
   * Cancel the class booking event and remove class id if this is the only booking.
   */
  cancelBookClassEvent(user_id, event_key, class_id, booking_id, reset, callback) {
    if (reset === true) {
      var query = "UPDATE events SET bookings = array_remove(bookings, \'" + booking_id + "\'), status='open', class_id = null WHERE user_id = \'" + user_id + "\' AND event_key = \'" + event_key + "\';"
    } else {
      var query = "UPDATE events SET bookings = array_remove(bookings, \'" + booking_id + "\') WHERE user_id = \'" + user_id + "\' AND event_key = \'" + event_key + "\';"
    }
    this.execute(query, function(e,r) {
      callback(e,r);
    });
  }

  addEvent(user_id, schedule_key, event_key, event_num, course, start_date, end_date, commencement_date, timezone_offset, status, callback) {
    try {
      commencement_date = new Date(commencement_date).getTime();
    } catch(e) {
      console.log(e);
    }
    try {
      start_date = new Date(start_date).getTime();
      end_date = new Date(end_date).getTime();
    } catch(e) {
      console.log(e);
    }
    var query = "INSERT INTO events(user_id, schedule_key, event_key, event_num, course, start_date, end_date, commencement_date, timezone_offset, status)"
      + " VALUES("
      + "\'" + user_id + "\',"
      + "\'" + schedule_key + "\',"
      + "\'" + event_key + "\',"
      + "\'" + event_num + "\',"
      + "\'" + course + "\',"
      + start_date + ","
      + end_date + ","
      + commencement_date + ","
      + "\'" + timezone_offset + "\',"
      + "\'open\');"
    this.execute(query, function(e,r) {
      if (e) {
        callback(e,false);
      } else if (r) {
        if (r.count > 0) {
          callback(e,true);
        } else {
          callback(e,false);
        }
      } else {
        callback(e,null);
      }
    });
  }

  deleteEvent(user_id, event_key, callback) {
    var query = "DELETE FROM events WHERE user_id=\'" + user_id + "\' AND event_key = \'" + event_key + "\';"
    this.execute(query, function(e,r) {
      if (r) {
        if (r.count > 0) {
          callback(e,true);
        } else {
          callback(e,false);
        }
      } else {
        callback(e,null);
      }
    });
  }

  deleteEvents(user_id, schedule_key, callback) {
    var query = "DELETE FROM events WHERE user_id=\'" + user_id + "\' AND schedule_key = \'" + schedule_key + "\';"
    this.execute(query, function(e,r) {
      if (r) {
        if (r.count > 0) {
          callback(e,true);
        } else {
          callback(e,false);
        }
      } else {
        callback(e,null);
      }
    });
  }

  /*
   * Bookings functions
   */

  addBooking(student_id, teacher_id, class_id, classtitle, classtype, classcity, classvia, events, request_status, timezone_offset, commencement_date, booking_date, booking_amount, currency, payment_source, language, callback) {
    try {
      commencement_date = new Date(commencement_date).getTime();
    } catch(e) {
      console.log(e);
    }
    try {
      booking_date = new Date(booking_date).getTime();
    } catch(e) {
      console.log(e);
    }
    var query = "INSERT INTO bookings(student_id, teacher_id, class_id, classtitle, classtype, classcity, classvia, language, events, request_status, timezone_offset, commencement_date, booking_date, booking_amount, currency, payment_source)"
      + " VALUES("
      + "\'" + student_id + "\',"
      + "\'" + teacher_id + "\',"
      + "\'" + class_id + "\',"
      + "\'" + classtitle + "\',"
      + "\'" + classtype + "\',"
      + "\'" + classcity + "\',"
      + "\'" + classvia + "\',"
      + "\'" + language + "\',"
      + "ARRAY[\'" + events.join('\',\'') + "\'],"
      + "\'" + request_status + "\',"
      + "\'" + timezone_offset + "\',"
      + commencement_date + ","
      + booking_date + ","
      + "\'" + booking_amount + "\',"
      + "\'" + currency + "\',"
      + "\'" + payment_source + "\') RETURNING id;"
    this.execute(query, function(e,r) {
      if (r[0]) {
        callback(e,r[0].id);
      } else {
        callback('no booking id returned');
      }
    });
  }

  updateBookingStatus(id, user_id, status, callback) {
    var query = "UPDATE bookings SET request_status=\'" + status + "\' WHERE teacher_id=\'" + user_id + "\' AND id=\'" + id + "\';";
    this.execute(query, function(e,r) {
      callback(e,r);
    });
  }

  addTuition(event_key, class_id, booking_id, student_id, teacher_id, start_date, end_date, callback) {
    try {
      start_date = new Date(start_date).getTime();
      end_date = new Date(end_date).getTime();
    } catch(e) {
      console.log(e);
    }
    var query = "INSERT INTO tuitions(class_id, booking_id, student_id, teacher_id, start_date, end_date, event_key, student_status, teacher_status)"
      + " VALUES("
      + "\'" + class_id + "\',"
      + "\'" + booking_id + "\',"
      + "\'" + student_id + "\',"
      + "\'" + teacher_id + "\',"
      + start_date + ","
      + end_date + ","
      + "\'" + event_key + "\',"
      + "\'" + 'upcoming' + "\',"
      + "\'" + 'upcoming' + "\')"
    this.execute(query, function(e,r) {
      if (e) {
        callback(e,false);
      } else if (r) {
        callback(e,true);
      } else {
        callback(null,false);
      }
    });
  }

  getTuitionsByBookingId(booking_id, callback) {
    var query = "SELECT * FROM tuitions WHERE booking_id = \'" + booking_id + "\';";
    this.execute(query, function(e,r) {
      if (e) {
        console.log(e);
        callback([]);
      } else {
        if (r) {
          var tuitions = [];
          if (r.length > 0) {
            for (i in r) {
              var tuition = {};
              tuition.class_id = r[i].class_id,
                tuition.student_id = r[i].student_id,
                tuition.teacher_id = r[i].teacher_id,
                tuition.student_rating = r[i].student_rating,
                tuition.teacher_rating = r[i].teacher_rating,
                tuition.student_comment = r[i].student_comment,
                tuition.teacher_comment = r[i].teacher_comment,
                tuition.event_key = r[i].event_key,
                tuition.start_date = r[i].start_date,
                tuition.end_date = r[i].end_date,
                tuition.student_status = r[i].student_status,
                tuition.teacher_status = r[i].teacher_status,
                tuition.flag = r[i].flag
              tuitions.push(tuition);
            }
            callback(tuitions);
          } else {
            callback([]);
          }
        } else {
          callback([]);
        }
      }
    });
  }

  updateTuitionStatus(class_id, booking_id, event_key, status, callback) {
    var query = "UPDATE tuitions SET student_status=\'" + status + "\', teacher_status=\'" + status + "\' WHERE class_id=\'" + class_id + "\' AND booking_id=\'" + booking_id + "\' AND  event_key=\'" + event_key + "\' AND student_status!=\'completed\' AND teacher_status!=\'completed\';";
    this.execute(query, function(e,r) {
      callback(e,r);
    });
  }

  getBookingById(booking_id, callback) {
    var query = "SELECT * FROM bookings WHERE id=\'" + booking_id + "\';"
    this.execute(query, function(e,r) {
      if (r) {
        if (r[0]) {
          var booking = {};
          booking.booking_id = r[0].id;
          booking.student_id = r[0].student_id;
          booking.teacher_id = r[0].teacher_id;
          booking.class_id = r[0].class_id;
          booking.classtype = r[0].classtype;
          booking.classcity = r[0].classcity;
          booking.classvia = r[0].classvia;
          booking.classtitle = r[0].classtitle;
          booking.classtype = r[0].classtype;
          booking.language = r[0].language;
          booking.events = r[0].events;
          booking.request_status = r[0].request_status;
          booking.timezone_offset = r[0].timezone_offset;
          booking.commencement_date = r[0].commencement_date;
          booking.booking_date = r[0].booking_date;
          booking.booking_amount = r[0].booking_amount;
          booking.currency = r[0].currency;
          booking.payment_source = r[0].payment_source;
          callback(e,booking);
        } else {
          callback(e,null);
        }
      } else {
        callback(e,null);
      }
    });
  }

  /*
   * Same as above but this function also returns the payment info(only for internal use)
   */
  getBookingByIdWithCard(booking_id, callback) {
    var query = "SELECT * FROM bookings WHERE id=\'" + booking_id + "\';"
    this.execute(query, function(e,r) {
      if (r) {
        if (r[0]) {
          var booking = {};
          booking.booking_id = r[0].id;
          booking.student_id = r[0].student_id;
          booking.teacher_id = r[0].teacher_id;
          booking.class_id = r[0].class_id;
          booking.classtype = r[0].classtype;
          booking.classcity = r[0].classcity;
          booking.classvia = r[0].classvia;
          booking.classtitle = r[0].classtitle;
          booking.classtype = r[0].classtype;
          booking.language = r[0].language;
          booking.events = r[0].events;
          booking.request_status = r[0].request_status;
          booking.timezone_offset = r[0].timezone_offset;
          booking.commencement_date = r[0].commencement_date;
          booking.booking_date = r[0].booking_date;
          booking.booking_amount = r[0].booking_amount;
          booking.currency = r[0].currency;
          booking.payment_source = r[0].payment_source;
          callback(e,booking);
        } else {
          callback(e,null);
        }
      } else {
        callback(e,null);
      }
    });
  }

  getBookingsByIds(user_id, booking_ids, callback) {
    var listString;
    if (booking_ids) {
      if (booking_ids.length > 1) {
        listString = booking_ids.join('\',\'');
      } else {
        listString = booking_ids;
      }
    } else {
      callback();
    }
    var query = "SELECT * FROM bookings WHERE id IN (\'" + listString + "\') AND teacher_id=\'" + user_id + "\';"
    this.execute(query, function(e,r) {
      if (r) {
        var bookings = [];
        for (var i=0; i<r.length;i++) {
          if (r[i]) {
            var booking = {};
            booking.booking_id = r[i].id;
            booking.student_id = r[i].student_id;
            booking.teacher_id = r[i].teacher_id;
            booking.class_id = r[i].class_id;
            booking.classtype = r[i].classtype;
            booking.classcity = r[i].classcity;
            booking.classvia = r[i].classvia;
            booking.classtitle = r[i].classtitle;
            booking.classtype = r[i].classtype;
            booking.events = r[i].events;
            booking.request_status = r[i].request_status;
            booking.timezone_offset = r[i].timezone_offset;
            booking.commencement_date = r[i].commencement_date;
            booking.booking_date = r[i].booking_date;
            booking.booking_amount = r[i].booking_amount;
            booking.currency = r[i].currency;
            booking.payment_source = r[i].payment_source;
            bookings.push(booking);
          }
        }
        callback(e,bookings);
      } else {
        callback(e,null);
      }
    });
  }

  getUserBookings(user_id, userview, callback) {
    var query;
    if (userview === "teacher") {
      query = "SELECT bookings.*, users.username FROM bookings INNER JOIN users ON users.id = bookings.student_id WHERE teacher_id=\'" + user_id + "\';"
    } else if (userview === "student") {
      query = "SELECT bookings.*, users.username FROM bookings INNER JOIN users ON users.id = bookings.teacher_id WHERE student_id=\'" + user_id + "\';"
    } else {
      callback('no userview defined',null);
    }
    this.execute(query, function(e,r) {
      if (r) {
        if (r.length > 0) {
          var bookings = [];
          for (i in r) {
            var booking = {};
            booking.booking_id = r[i].id;
            booking.username = r[i].username;
            booking.student_id = r[i].student_id;
            booking.teacher_id = r[i].teacher_id;
            booking.class_id = r[i].class_id;
            booking.classtype = r[i].classtype;
            booking.classcity = r[i].classcity;
            booking.classvia = r[i].classvia;
            booking.classtitle = r[i].classtitle;
            booking.classtype = r[i].classtype;
            booking.events = r[i].events;
            booking.request_status = r[i].request_status;
            booking.timezone_offset = r[i].timezone_offset;
            booking.commencement_date = r[i].commencement_date;
            booking.booking_date = r[i].booking_date;
            booking.booking_amount = r[i].booking_amount;
            booking.currency = r[i].currency;
            bookings.push(booking);
          }
          callback(e,bookings);
        } else {
          callback(e,null);
        }
      } else {
        callback(e,null);
      }
    });
  }

  hasUserActiveClassBookings(user_id, class_id, callback) {
    var query = "SELECT COUNT(id) FROM bookings WHERE student_id=\'" + user_id + "\' OR teacher_id=\'" + user_id + "\' AND class_id=\'" + class_id + "\' AND request_status IN ('requested','confirmed','inprogress','feedback');"
    this.execute(query, function(e,r) {
      if (e) {
        console.log(e);
        callback(false);
      } else {
        if (parseInt(r[0].count) > 0) {
          callback(true);
        } else {
          callback(false);
        }
      }
    });
  }

  getActiveBookingIds(callback) {
    var query = "SELECT id FROM bookings WHERE request_status IN ('requested','confirmed','inprogress','feedback');"
    this.execute(query, function(e,r) {
      if (e) {
        console.log(e);
        callback(e,[]);
      } else {
        if (r) {
          var ids=[];
          for (var i in r) {
            ids.push(r[i].id);
          }
          callback(ids);
        } else {
          callback([]);
        }
      }
    });
  }

  addStripeCustomer(id, user_id, account_balance, created, currency, default_source, description, email, callback) {
    var query = "INSERT INTO stripe_customers(customer_id, user_id, account_balance, created, currency, default_source, description, email)"
      + " VALUES("
      + "\'" + id + "\',"
      + "\'" + user_id + "\',"
      + "\'" + account_balance + "\',"
      + created + ","
      + "\'" + currency + "\',"
      + "\'" + default_source + "\',"
      + "\'" + description + "\',"
      + "\'" + email + "\')"
      + " ON CONFLICT (user_id) DO UPDATE SET"
      + " customer_id = \'" + id + "\',"
      + " account_balance = \'" + account_balance + "\',"
      + " created = " + created + ","
      + " currency = \'" + currency + "\',"
      + " default_source = \'" + default_source + "\',"
      + " description = \'" + description + "\',"
      + " email = \'" + email + "\';";
    this.execute(query, function(e,r) {
      if (e) {
        callback(e,false);
      } else if (r) {
        callback(e,true);
      } else {
        callback(null,false);
      }
    });
  }

  /*
   * Return all stripe customers related to the supplied userid
   */
  getStripeCustomerIdByUserId(user_id, callback) {
    var query = "SELECT customer_id FROM stripe_customers WHERE user_id = \'" + user_id + "\';";
    this.execute(query, function(e,r) {
      if (e) {
        console.log(e);
        callback(e, "");
      } else {
        if (r) {
          if (r[0]) {
            callback(e, r[0].customer_id);
          } else {
            callback(e, "");
          }
        } else {
          callback(e, "");
        }
      }
    });
  }

  /*
   * Return all stripe customers related to the supplied userid
   */
  getStripeCustomerByUserId(user_id, callback) {
    var query = "SELECT * FROM stripe_customers WHERE user_id = \'" + user_id + "\';";
    this.execute(query, function(e,r) {
      if (e) {
        console.log(e);
        callback(e, {});
      } else {
        if (r) {
          if (r[0]) {
            var customer = {};
            customer.customer_id = r[0].customer_id;
            customer.user_id = r[0].user_id;
            customer.object = r[0].object;
            customer.account_balance = r[0].account_balance;
            customer.created = r[0].created;
            customer.currency = r[0].currency;
            customer.default_source = r[0].default_source;
            customer.description = r[0].description;
            customer.email = r[0].email;
            customer.json_object = r[0].json_object;
            callback(null,customer);
          } else {
            callback(e, {});
          }
        } else {
          callback(e, {});
        }
      }
    });
  }

  addStripeSource(customer_id, user_id, source, callback) {
    var query = "INSERT INTO stripe_sources(customer_id, user_id, source)"
      + " VALUES("
      + "\'" + customer_id + "\',"
      + "\'" + user_id + "\',"
      + "\'" + source + "\');"
    this.execute(query, function(e,r) {
      if (e) {
        console.log(e);
      } else {
        callback(true);
      }
    });
  }

  addStripeCharge(charge_id, amount, currency, customer_id, student_id, teacher_id, booking_id, classtitle, source, date, status, failure_code, failure_message, receipt_url, callback) {
    try {
      date = new Date(date).getTime();
    } catch(e) {
      console.log(e);
    }
    var query = "INSERT INTO stripe_charges(charge_id, amount, currency, customer_id, student_id, teacher_id, booking_id, classtitle, source, date, status, failure_code, failure_message, receipt_url)"
      + " VALUES("
      + "\'" + charge_id + "\',"
      + "\'" + amount + "\',"
      + "\'" + currency + "\',"
      + "\'" + customer_id + "\',"
      + "\'" + student_id + "\',"
      + "\'" + teacher_id + "\',"
      + "\'" + booking_id + "\',"
      + "\'" + classtitle + "\',"
      + "\'" + source + "\',"
      + date + ","
      + "\'" + status + "\',"
      + "\'" + failure_code + "\',"
      + "\'" + failure_message + "\',"
      + "\'" + receipt_url + "\')"
      + " ON CONFLICT (charge_id) DO NOTHING;"
    this.execute(query, function(e,r) {
      if (e) {
        console.log(e);
        callback(false);
      } else {
        callback(true);
      }
    });
  }

  /*
   * Return all stripe charges for a user
   */
  getStripeChargesByUserId(user_id, callback) {
    var query = "SELECT * FROM stripe_charges WHERE teacher_id = \'" + user_id + "\' OR student_id = \'" + user_id + "\';";
    this.execute(query, function(e,r) {
      if (e) {
        console.log(e);
        callback(e, []);
      } else {
        if (r) {
          var charges = [];
          if (r.length > 0) {
            for (i in r) {
              var charge = {};
              charge.charge_id = r[i].charge_id,
                charge.amount = r[i].amount,
                charge.currency = r[i].currency,
                charge.customer_id = r[i].customer_id,
                charge.student_id = r[i].student_id,
                charge.booking_id = r[i].booking_id,
                charge.classtitle = r[i].classtitle,
                charge.teacher_id = r[i].teacher_id,
                charge.source = r[i].source,
                charge.date = r[i].date,
                charges.push(charge);
            }
            callback(charges);
          } else {
            callback(e, []);
          }
        } else {
          callback(e, []);
        }
      }
    });
  }

  getCanonicalUrls(callback) {
    var query = "SELECT canonical_url FROM profile UNION SELECT canonical_url FROM classes;";
    this.execute(query, function(e,r) {
      if (e) {
        console.log(e);
        callback(e, []);
      } else {
        var canonical_urls = [];
        for (i in r) {
          canonical_urls.push('https://www.edivy.com' + r[i].canonical_url);
        }
        callback(null, canonical_urls);
      }
    });
  }

  saveAccount(user_id, bank_name, branch_name, holder_address_1, holder_address_2, holder_city, holder_country, holder_name, holder_zip_code, number, sort_code, type, callback) {
    var query = "INSERT INTO accounts(user_id, bank_name, branch_name, holder_address_1, holder_address_2, holder_city, holder_country, holder_name, holder_zip_code, number, sort_code, type)"
      + " VALUES("
      + "\'" + user_id + "\',"
      + this.format(bank_name) + ","
      + this.format(branch_name) + ","
      + this.format(holder_address_1) + ","
      + this.format(holder_address_2) + ","
      + this.format(holder_city) + ","
      + this.format(holder_country) + ","
      + this.format(holder_name) + ","
      + this.format(holder_zip_code) + ","
      + this.format(number) + ","
      + this.format(sort_code) + ","
      + this.format(type) + ")"
      + " ON CONFLICT (user_id) DO UPDATE SET"
      + " bank_name = " + this.format(bank_name) + ","
      + " branch_name = " + this.format(branch_name) + ","
      + " holder_address_1 = " + this.format(holder_address_1) + ","
      + " holder_address_2 = " + this.format(holder_address_2) + ","
      + " holder_city = " + this.format(holder_city) + ","
      + " holder_country = " + this.format(holder_country) + ","
      + " holder_name = " + this.format(holder_name) + ","
      + " holder_zip_code = " + this.format(holder_zip_code) + ","
      + " number = " + this.format(number) + ","
      + " sort_code = " + this.format(sort_code) + ","
      + " type = " + this.format(type) + ";";
    this.execute(query, function(e,r) {
      if (e) {
        callback(e,false);
      } else if (r) {
        callback(e,true);
      } else {
        callback(null,false);
      }
    });
  }

  getAccount(user_id, callback) {
    var query = "SELECT * FROM accounts WHERE user_id = \'" + user_id + "\';";
    this.execute(query, function(e,r) {
      if (e) {
        console.log(e);
        callback();
      } else {
        if (r[0]) {
          callback(null, r[0]);
        } else {
          callback();
        }
      }
    });
  }

}

function getDataType(dt) {
  switch(dt) {
    case 'serial':
      return 'SERIAL';
    case 'bigserial': 
      return 'BIGSERIAL';
    case 'int': 
      return 'INT';
    case 'bigint': 
      return 'BIGINT';
    case 'decimal':
      return 'NUMERIC(16,2)';
    case 'bigdecimal':
      return 'NUMERIC(32,16)';
    case 'real': 
      return 'REAL';
    case 'date': 
      return 'DATE';
    case 'varchar_2': 
      return 'VARCHAR(2)';
    case 'varchar_8': 
      return 'VARCHAR(8)';
    case 'varchar_64': 
      return 'VARCHAR(64)';
    case 'varchar_2048': 
      return 'VARCHAR(2048)';
    case 'varchar_8192': 
      return 'VARCHAR(8192)';
    case 'text': 
      return 'TEXT';
    default:
      return 'SERIAL';
  }
}

module.exports = Postgres;
