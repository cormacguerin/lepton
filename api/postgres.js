/*
 * Wrapper for Postgres.
 */

const { Pool } = require('pg');
var fs = require('fs')

class Postgres {

  constructor(db) {

    if (db) {

      console.log(db)

      this.pool = new Pool({
        statement_timeout: 60000,
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
    //console.log(statement);
    //console.log(values);

    (async () => {
      const client = await this.pool.connect()
      console.log('database connected')
      try {
        const reply = await client.query(statement, values)
        this.logger.write(statement + "\n");
        callback(null, reply.rows)
      } catch (e) {
        callback(null, null)
        console.log(e)
      } finally {
        await client.release();
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
      } catch (e) {
        console.log(e);
        callback(null, null)
      } finally {
        await client.release();
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
            var et = new Date().getTime();
            var totaltime = et-it;
            console.log("promises finished in " + totaltime + "ms");
            callback(null, r);
          })
          .catch((e) => {
            console.log('error pushing statements')
            console.log(e);
            client.query("ROLLBACK");
            callback(e);
          });
        }
        promisePush();
      } catch(e) {
        console.log('error in rollback');
        console.log(e);
        await client.query("ROLLBACK");
      } finally {
        console.log("finally");
        await client.release();
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

    this.execute(query, null, function(e,r) {
      if (e) {
        console.log(e)
        callback();
      } else {
        callback(r);
      }
    });
  }

  addIndexSchema(callback) {
    var query = fs.readFileSync('./server/index_schema.psql').toString();

    this.execute(query, null, function(e,r) {
      if (e) {
        console.log(e)
        callback(e,r)
      } else {
        callback(e,r)
      }
    })
  }

  /*
   * Add database
   * We prepend database names with the user_id to ensure no duplicates.
   * Separately we tracks user databases in a database table.
   */
  addDatabase(user, database, callback) {

    var vm = this;
    var query = "CREATE DATABASE \"" + database +  "\" ENCODING UTF8;"
    console.log(query)
    this.execute(query, null, function(e,r) {
      if (e) {
        callback(e);
      } else {
        var query = "INSERT INTO databases(database,owner,reader,writer) VALUES($1,$2,$2,$2);"
        vm.execute(query, [database, user], function(e,r) {
          callback(e, r);
        });
      }
    });
  }

  //  todo , load from server/index script instead (like we do for admin creation)
  /*
  addNgramTable(callback) {
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
      promises.push(new Promise((pr, pe) => {
        var query = "CREATE TABLE stop_suggest ("
          + "id SERIAL PRIMARY KEY,"
          + "lang VARCHAR(2),"
          + "gram VARCHAR(1024) NOT NULL UNIQUE,"
          + "stop VARCHAR(32),"
          + "idf real,"
          + "UNIQUE (lang,gram)"
          + ");"
        vm.execute(query, null, function(e,r) {
          if (e) {
            pe(e)
          } else {
            pr(r)
          }
        });
      }));
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
    */

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
      + "lang VARCHAR(2),"
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

  getDataSetData(database, tablename, callback) {
    var query = "SELECT data FROM tables WHERE database = (SELECT id FROM databases WHERE database = $1) and tablename = $2;"
    this.execute(query, [database, tablename], function(e,r) {
      if (r[0].data) {
        try {
          let q = JSON.parse(r[0].data)
          callback(e,q.query);
        } catch(e) {
          callback(e)
        }
      } else {
        callback('{"status":"invalid data"}')
      }
    });
  }

  getTables(user, database, callback) {
    var query = "SELECT tablename, type, data FROM tables WHERE database = (SELECT id FROM databases WHERE database = $1) AND owner = $2;"
    this.execute(query, [database, user], function(e,r) {
      callback(e,r);
    });
  }

  getTableSchema(table, callback) {
    var query = "SELECT column_name, data_type, character_maximum_length FROM INFORMATION_SCHEMA.COLUMNS WHERE table_name = $1;"
    this.execute(query, [table], function(e,r) {
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
    var query = "SELECT d.database, t.tablename AS table, array_agg(tx._column ORDER BY t.tablename) AS column, display_field, indexing from text_tables_index tx INNER JOIN databases d on d.id = tx.database INNER JOIN tables t on tx._table = t.id WHERE d.owner = $1 AND indexing = true GROUP BY d.database, t.tablename, display_field, indexing ORDER BY d.database;"
    this.execute(query, [user_id], function(e,r) {
      callback(e,r);
    });
  }

  getServingTables(user_id, callback) {
    var query = "SELECT d.database, t.tablename AS table, (SELECT (array_agg(_column))) AS column FROM text_tables_index tx INNER JOIN databases d on d.id = tx.database INNER JOIN tables t on tx._table = t.id WHERE d.owner = $1 AND tx.serving = True AND tx._column is not null GROUP BY d.database, t.tablename;"
    // var query = "SELECT d.database, t.tablename AS table, (SELECT (array_agg(_column))) AS column FROM text_tables_index tx INNER JOIN databases d on d.id = tx.database INNER JOIN tables t on tx._table = t.id WHERE d.owner = $1 AND tx._column is not null AND tx.serving = true GROUP BY d.database, t.tablename;"
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
      + " ON CONFLICT ON CONSTRAINT text_tables_index_database__table__column DO UPDATE SET"
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

  addCrawlerUrl(t,url,callback) {
    var query = "INSERT INTO crawler_urls(_table,url) VALUES ($1,$2) ON CONFLICT DO NOTHING";
    console.log('t ' + t)
    console.log('rul ' + url)

    var values = [t,url]

    this.execute(query, values, function(e,r) {
      console.log(e);
      console.log(r);
      callback(e,r);
    });
  }

  deleteCrawlerUrl(t,url,callback) {
    var query = "DELETE FROM crawler_urls WHERE _table = $1 AND url = $2";


    var values = [t,url]

    this.execute(query, values, function(e,r) {
      console.log(e);
      console.log(r);
      callback(e,r);
    });
  }

  getCrawlerUrls(callback) {
    var query = "SELECT url, _table FROM crawler_urls";

    this.execute(query, null, function(e,r) {
      console.log(e);
      console.log(r);
      callback(e,r);
    });
  }

  addModel(u,l,m, callback) {
    var query = "INSERT INTO ml_models(model,lang,owner) VALUES($1,$2,$3)"

    var values = [m,l,u]

    this.execute(query, values, function(e,r) {
      console.log(e);
      console.log(r);
      callback(e,r);
    });
  }

  saveModel(u,i,l,m,p,d, callback) {
    var query = "UPDATE ml_models(model,lang,program,dataset) VALUES($1,$2,$3,$4) WHERE owner = $5 and id = $6"

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
