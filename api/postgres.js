/*
 * Wrapper for Postgres.
 */

const { Pool } = require('pg');

class Postgres {

  constructor(db) {

    if (db) {

      var fs = require('fs')

      if (db.database === 'admin') {

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
          } else {
            password = line;
          }
        });

        this.pool = new Pool({
          user: 'postgres',
          host: 'localhost',
          database: 'admin',
          password: password,
          port: 5432,
          idleTimeoutMillis: 60000,
        });
      } else {
        this.pool = new Pool({
          user: 'postgres',
          host: 'localhost',
          database: db.database,
          password: db.password,
          port: 5432,
        });
      }

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

  execute(statement, callback) {
    console.log(statement);
    (async () => {
      const client = await this.pool.connect()
      try {
        const reply = await client.query(statement)
        // console.log(reply.rows);
        // console.log(' - - ');
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

  pexecute(statement, values, callback) {
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
            /*
            console.log('v');
            console.log(v);
            console.log('values[v]');
            console.log(values[v]);
            console.log('Object.values(values[v])');
            console.log(Object.values(values[v]));
            console.log(' - - - - ');
            */
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
   * Add database
   */
  addDatabase(database, callback) {

    var query = "CREATE DATABASE " 
      + database
      + " WITH OWNER postgres"
      + " ENCODING 'UTF8'"
      + " LC_COLLATE = 'en_US.UTF-8'"
      + " LC_CTYPE = 'en_US.UTF-8'"

    this.execute(query, function(e,r) {
      callback(e, r);
    });
  }

  /*
   * Create database
   */
  createTable(table, column, datatype, callback) {

    var query = "CREATE TABLE " 
      + table
      + "(" 
      + column
      + " "
      + getDataType(datatype)
      + " PRIMARY KEY);"

    this.execute(query, function(e,r) {
      callback(e, r);
    });
  }

  /*
   * Add database
   */
  addTableColumn(table, column, datatype, callback) {

    var query = "ALTER TABLE " 
      + table
      + " ADD COLUMN " 
      + column
      + " "
      + getDataType(datatype)

    this.execute(query, function(e,r) {
      callback(e, r);
    });
  }

  /*
   * Rename
   */
  renameTableColumn(table, column, old_column, callback) {

    var query = "ALTER TABLE "
      + table
      + " RENAME COLUMN "
      + old_column
      + " TO "
      + column

    this.execute(query, function(e,r) {
      callback(e, r);
    });
  }

  /*
   * Rename
   */
  setTableColumnDataType(table, column, datatype, callback) {

    var query = "ALTER TABLE "
      + table
      + " ALTER COLUMN "
      + column
      + " TYPE "
      + getDataType(datatype)

    this.execute(query, function(e,r) {
      callback(e, r);
    });
  }

  /*
   * Add database
   */
  deleteTableColumn(table, column, callback) {

    var query = "ALTER TABLE " 
      + table
      + " DROP COLUMN " 
      + column

    this.execute(query, function(e,r) {
      callback(e, r);
    });
  }

  addTableData(table, data, callback) {
    var it = new Date().getTime();
    console.log("ADD TABLE DATA " + table);
    var pkey = "SELECT a.attname, format_type(a.atttypid, a.atttypmod) AS data_type"
      + " FROM   pg_index i"
      + " JOIN   pg_attribute a ON a.attrelid = i.indrelid"
      + " AND a.attnum = ANY(i.indkey) "
      + " WHERE  i.indrelid = '"
      + table
      + "'::regclass"
      + " AND    i.indisprimary;"
    var this_ = this;
    this.execute(pkey, function(e,r) {
      if (e) {
        console.log(e);
        return callback(e);
      }
      var primary_key;
      if (r[0]) {
        primary_key = r[0].attname;
      } else {
        return callback('unknown primary key, please provide a primary key.');
      }
      console.log('primary_key : ' + primary_key);
      var result = {
        status:'',
        errors: [],
        results: []
      }

      var keys = Object.keys(data[0]);
      var insert_prep = '';
      for (var i=1; i<=keys.length; i++) {
        insert_prep += '$' + i.toString();
        if (i < keys.length) {
          insert_prep += ', ';
        }
      }
      var conflict_prep = '';
      for (var i=1; i<=keys.length; i++) {
        conflict_prep += keys[i-1];
        conflict_prep += ' = ';
        conflict_prep += '$' + i.toString();
        if (i < keys.length) {
          conflict_prep += ', ';
        }
      }
      var insert = "INSERT INTO "
        + table
        + " ("
        + keys
        + ") VALUES("
        + insert_prep
        + ")"
        + " ON CONFLICT ("
        + primary_key
        + ") DO UPDATE SET "
        + conflict_prep 
        + ";"

      console.log(insert);
      this_.pexecute(insert, data, function(e,r) {
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
        return callback(null, result);
      });
    });
  }

  /*
   * Delete database
   */
  deleteDatabase(database, callback) {

    var disable = "UPDATE pg_database SET datallowconn = 'false' WHERE datname = \'" +database+"\';"
    var disconnect = "SELECT pg_terminate_backend(pid) FROM pg_stat_activity WHERE datname = \'" +database+"\';"
    var drop = "DROP DATABASE " + database

    var this_ = this;
    this.execute(disable, function(e,r) {
      if (e) {
        console.log(e);
      }
      this_.execute(disconnect, function(e,r) {
        var this__ = this_;
        if (e) {
          console.log(e);
        }
        this__.execute(drop, function(e,r) {
          callback(e, r);
        });
      });
    });
  }

  /*
   * User Functions
   */
  addUser(username, email, password, default_view, facebook_user_id, facebook_info, account_type, account_status, confirm_code, color_code, role, callback) {

    var query = "INSERT INTO users(username, email, password, default_view, facebook_user_id, facebook_info, account_type, account_status, confirm_code, color_code, role, created_date)"
      + " VALUES("
      + "\'" + username + "\',"
      + "\'" + email + "\',"
      + "\'" + password + "\',"
      + "\'" + default_view + "\',"
      + "\'" + facebook_user_id + "\',"
      + "\'" + facebook_info + "\',"
      + "\'" + account_status + "\',"
      + "\'" + account_type + "\',"
      + "\'" + confirm_code + "\',"
      + "\'" + color_code + "\',"
      + "\'" + role + "\',"
      + "\'" + new Date().getTime() + "\') RETURNING id;"
    this.execute(query, function(e,r) {
      if (e) {
        callback(e);
      } else {
        callback(null,r[0].id);
      }
    });
  }

  confirmUser(user_id, callback) {
    var query = "UPDATE users SET account_status = 'confirmed' WHERE id = \'" + user_id + "\';"
    this.execute(query, function(e,r) {
      callback(e,r);
    });
  }

  resetConfirmCode(email, confirm_code, callback) {
    var query = "UPDATE users SET confirm_code = \'" + confirm_code + "\' WHERE email = \'" + email + "\';"
    this.execute(query, function(e,r) {
      if (e) {
        callback(false);
      } else {
        callback(true);
      }
    });
  }

  resetPassword(email, password, confirm_code, callback) {
    var query = "UPDATE users SET password = \'" + password + "\' WHERE email = \'" + email + "\' AND confirm_code =  \'" + confirm_code + "\';"
    this.execute(query, function(e,r) {
      if (e) {
        callback(false);
      } else {
        callback(true);
      }
    });
  }

  getUserClients(callback) {
    var query = "SELECT * FROM user_clients;"
    this.execute(query, function(e,r) {
      callback(e,r);
    });
  }

  checkTableForValue(table, column, value, callback) {
    var query = "SELECT COUNT(" + column + ") FROM " + table + " WHERE " + column + " = '" + value + "';"
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

  addUserClient(client, callback) {
    var query = "INSERT INTO user_clients (client_id, user_id, success, created_date, active_date, token, message)"
      + " VALUES("
      + "\'" + client.client_id + "\',"
      + "\'" + client.user_id + "\',"
      + "\'" + client.success + "\',"
      + "\'" + new Date().getTime() + "\',"
      + "\'" + new Date().getTime() + "\',"
      + "\'" + client.token + "\',"
      + "\'" + client.message + "\')"
      + " ON CONFLICT (user_id,client_id) DO UPDATE SET"
      + " active_date = " + new Date().getTime() + ","
      + " token = \'" + client.token + "\',"
      + " message = \'" + client.message + "\';";
    this.execute(query, function(e,r) {
      callback(e,r);
    });
  }

  removeUserClient(client, callback) {
    var query = "DELETE FROM user_clients WHERE client_id = "
      + "\'" + client.client_id + "\' AND user_id = "
      + "\'" + client.user_id + "\';"
    this.execute(query, function(e,r) {
      callback(e,r);
    });
  }

  getUserInfo(key, values, callback) {
    var query;
    if (Number.isInteger(parseInt(key))) {
      query = "SELECT " + values.join(',') + " FROM users WHERE id = '" + key + "';"
    } else {
      query = "SELECT " + values.join(',') + " FROM users WHERE email = '" + key + "';"
    }
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

  getUsernameFromId(user_id, callback) {
    if (Number.isInteger(parseInt(user_id)) ) {
      var query = "SELECT username FROM users WHERE id = '" + user_id + "';"
      this.execute(query, function(e,r) {
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

  getDatabases(callback) {
    var query = "SELECT datname FROM pg_database WHERE datistemplate = false;"
    this.execute(query, function(e,r) {
      console.log(e);
      console.log(r);
      callback(e,r);
    });
  }

  getTables(callback) {
    var query = "SELECT table_name as tables FROM information_schema.tables WHERE table_type = 'BASE TABLE' AND table_schema NOT IN ('pg_catalog', 'information_schema');"
    this.execute(query, function(e,r) {
      console.log(e);
      console.log(r);
      callback(e,r);
    });
  }

  getTableSchema(database, table, callback) {
    var query = "SELECT column_name, data_type, character_maximum_length FROM INFORMATION_SCHEMA.COLUMNS WHERE udt_catalog='" + database + "' AND table_name = '" + table + "';"
    this.execute(query, function(e,r) {
      console.log(e);
      console.log(r);
      callback(e,r);
    });
  }

  setFTS(d,t,c,df,b,callback) {
    var query = "INSERT INTO text_tables_index(database,_table,_column,display_field,enable) VALUES("
      + "\'" + d + "\',"
      + "\'" + t + "\',"
      + "\'" + c + "\',"
      + "\'" + df + "\',"
      + "\'" + b + "\')"
      + " ON CONFLICT ON CONSTRAINT text_tables_index_database__table__column_key DO UPDATE SET "
      + " database = \'" + d + "\',"
      + " _table = \'" + t + "\',"
      + " _column = \'" + c + "\',"
      + " display_field = \'" + df + "\',"
      + " enable = \'" + b + "\';";

    this.execute(query, function(e,r) {
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
      return 'NUMERIC(32,8)';
    case 'real': 
      return 'REAL';
    case 'date': 
      return 'DATE';
    case 'varchar_64': 
      return 'VARCHAR(64)';
    case 'varchar_2048': 
      return 'VARCHAR(2048)';
    case 'text': 
      return 'TEXT';
    default:
      return 'SERIAL';
  }
}

module.exports = Postgres;