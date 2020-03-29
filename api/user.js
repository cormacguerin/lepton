var postgres = require('./postgres.js');
pg = new postgres({database:'admin'});

var async = require('async');

var crypto = require("crypto");

const url = require('url');

var aesgcm = require('../aesgcm256');

var jwt = require('jsonwebtoken');

var secret = new Buffer('secret', 'base64');

var userClients = {};

var qs = require('querystring');

exports.clients = userClients;

/*
 * Load any previous user clients.
 */
exports.loadExistingSessions = function() {
	pg.getUserClients(function(err, clients) {
		if (err){
			console.log("unable to retrieve user_clients");
			console.log(err);
		}
		for (i in clients) {
      if (!userClients[clients[i].user_id]) {
          userClients[clients[i].user_id] = {};
      }
			userClients[clients[i].user_id][clients[i].client_id] = clients[i];
		}
	});
}

/*
 * A function to authenticate a client.
 * This will provide the client a valid token.
 * At this point we expect that the client has
 * already completed all authentication checks.
 * an unauthenticated client should never get
 * here.
 */
var authenticate = function(user_id, req, res) {
	if (!user_id) {
		return;
	}
	var token = jwt.sign(user_id, secret);
	var client_id = genRandStr();
	var new_client = {
		success: true,
		user_id: user_id,
		client_id: client_id,
		created: new Date(),
		message: 'authenticated with token!',
		token: token
	};
	if (!(user_id in userClients)) {
		console.log("New client Auth");
		userClients[user_id]={};
	}
	if (!(client_id in userClients[user_id])) {
		console.log("New client Auth");
		userClients[user_id][client_id] = {};
	}
	userClients[user_id][client_id] = new_client;
	if (typeof res.cookie === 'function') {
		res.cookie('petcookie', new_client, { httpOnly: true });
	} else {
		console.log("something went wrong - res.cookie is not a function");
		res.status(401);
		res.json({
			authorized: false,
		});
		return;
	}
	pg.addUserClient(new_client, function(e, r) {
		if (e) {
			console.log(e);
			res.status(200);
			res.json({
				authorized: false,
				message: err
			});
		} else {
			res.status(200);
			res.json({
				authorized: true,
			});
		}
		return;
	});
}

var genRandStr = function() {
	return Math.round(Math.pow(36, 10)*Math.random()).toString(36);
}

exports.genApiKey = function() {
  const e1 = Buffer.from(genRandStr()).toString('base64').substr(0,8).toUpperCase();
  const e2 = Buffer.from(genRandStr()).toString('base64').substr(0,8).toUpperCase();
  const e3 = Buffer.from(genRandStr()).toString('base64').substr(0,8).toUpperCase();
  const e4 = Buffer.from(genRandStr()).toString('base64').substr(0,8).toUpperCase();
  console.log(e1)
  return `${e1}-${e2}-${e3}-${e4}`;
}

/*
 * Function to authorize user requests based 
 * on a cookie/stored signed token.
 */
exports.authorize = function(req, res, next) {
	// First check that the cookie is present. If not bail.
	if (req.cookies['petcookie'] === undefined) {
		console.log('petcookie is undefined!');
		res.json({
			authorized: false,
			message: "no cookie found",
			language: req.language,
			region: req.region
		});
		return;
	} else {
		var token;
		var user_id;
		var client_id;
		var stored_token;
		try {
			/*
			 * Each request should supply a valid token and
			 * user id. If not bail.
			 */
			token = req.cookies['petcookie'].token;
			user_id = req.cookies['petcookie'].user_id;
			client_id = req.cookies['petcookie'].client_id;
			if (!token || !user_id) {
				console.log("missing token or user_id");
				res.json({
					authorized: false,
					message: "invalid cookie",
					language: req.language,
					region: req.region
				});
				return;
			}
		} catch(e) {
			res.json({
				authorized: false,
				message: "invalid cookie",
				language: req.language,
				region: req.region
			});
			return;
		}
		/*
		 * At this point we should have a token to test.
		 * Grab the stored token for supplied user Id.
		 * Test that the supplied token matches it. If not bail.
		 */
		if (!userClients[user_id] || isEmptyObject(userClients[user_id])) {
			res.json({
				authorized: false,
				message: "no client for user"
			});
			return;
		}
		if (!userClients[user_id][client_id] || isEmptyObject(userClients[user_id][client_id])) {
			res.json({
				authorized: false,
				message: "no client for user"
			});
			return;
		}
		if (typeof userClients[user_id][client_id].token === 'string') {
			stored_token = userClients[user_id][client_id].token;
		} else {
			res.json({
				authorized: false,
				message: "no token for user"
			});
			return;
		}
		if (stored_token === token) {
			/*
			 * Final step, verify the token. If invalid bail.
			 * If valid we proceed to next.
			 */
			jwt.verify(token, secret, function(err_, decoded) {
				if (err_) {
					console.log("token verification failed");
					res.json({
						authorized: false,
						message: "token verification failed"
					});
				} else {
					req.decoded = decoded;    
					req.user_id = user_id;
					req.client_id = client_id;
					// Each request should have a client_id for SSE
					// If client supplies an unknown client_id, this might mean
					// we have lost it for some reason, eg. server restart.
					// In this case we should re-auth them.
					if (!(user_id in userClients)) {
						res.status(200);
						res.json({
							authorized: false
						});
						return;
					}
					next();
				}
			});
		} else {
			console.log("stored token does not match user token");
			res.json({
				authorized: false,
				message: "invalid token"
			});
		}
	}
};

/*
 * Function to authorize api requests based on the api key and header signature.
 * This function retrieves scopes and adds them into the request but it does not
 * do any scope checking.
 * initially I thought it would be nice to do all the scope checking here but
 * I found that it makes it less versitile as different endpoints may want to do 
 * different things. Instead it's the endpoints responsibility to refute if 
 * req.scope is not satisfied.
 */
exports.authorizeApi = function(req, res, next) {
  var parsedurl = url.parse(req.url, true);
  const auth_re = /^LT-HMAC-SHA256 /gi;
  const cred_re = /^Credential=/gi;
  const head_re = /^SignedHeaders=/gi;
  const sig_re = /^Signature=/gi;
  var errors = []
  var authorization_header = req.headers.authorization;
  var authcsv, autharr, credential, signed_headers, client_signature
  var key_id, key_datestamp, key_scope

  if (authorization_header.match(auth_re)) {
    authcsv = authorization_header.replace(auth_re,'')
  }
  var autharr = authcsv.split(',')
  if (autharr[0].match(cred_re,'')) {
    credential = autharr[0].replace(cred_re,'').split('/')
    if (credential.length !== 4) {
      errors.push('expected 4 components in credential ' + credential.length + ' found )');
    } else {
      key_id = credential[0]
      key_datestamp = credential[1]
      key_scope = credential[2]
    }
  } else {
    errors.push('credential not present in authorization header');
  }
  if (autharr[1].match(head_re,'')) {
    signed_headers = autharr[1].replace(head_re,'').split(';')
    if (!(signed_headers.includes('content-type') && signed_headers.includes('date') && signed_headers.includes('host'))) {
      errors.push('signed headers must include at least "content-type", "date" and "host"');
    }
  } else {
    errors.push('signed headers not present in authorization header');
  }
  if (autharr[2].match(sig_re,'')) {
    client_signature = autharr[2].replace(sig_re,'')
  } else {
    errors.push('signature not present in authorization header');
  }
  // process the method
  if (req.method !== 'POST' && req.method !== 'GET') {
    errors.push('only POST or GET is supported but neither was found in the request')
  }
  // process headers (afaik javascript sorts objects by default in unicode points so no sorting to do.)
  var header_keys = Object.keys(req.headers)
  var signed_headers_str = '';
  for (var i in signed_headers.sort()) {
    if (header_keys.includes(signed_headers[i]) === true) {
      var h = signed_headers[i] + ':' + req.headers[signed_headers[i]].toLowerCase() + '\n'
      signed_headers_str += h
    } else {
      errors.push('signed header ' + signed_headers[i] + 'listed in the authorization header not found')
    }
  }
  // process the request parameters
  var query_parameters = '';
  for (var i in parsedurl.query) {
    var v = i + '=' + encodeURIComponent(parsedurl.query[i]) + '&';
    query_parameters += v;
  }
  query_parameters = query_parameters.replace(/&$/, '')
  console.log('query_parameters')
  console.log(query_parameters)

  // at this point we should be sure that the host header is present
  var host = req.headers.host
  // get the path and check that it maches the one sent in credentials
  if (!parsedurl.pathname) {
    errors.push('bad path found, expected /<path> but found "' + parsedurl.pathname + '"')
  }
  if (errors.length > 0) {
    res.status(403)
    return res.json({error:errors})
  } else {
    errors = []
  }

  // now build the signing string from the above information
  signing_string = req.method + "\n"
                 + req.headers.host + "\n"
                 + parsedurl.pathname + "\n"
                 + query_parameters + "\n"
                 + signed_headers_str

  if (parsedurl.pathname === 'testApiKey') {
    console.log('API debug info');
    console.log(' - credential key id')
    console.log(key_id)
    console.log(' - credential key datestamp')
    console.log(key_datestamp)
    console.log(' - credential key scope')
    console.log(key_scope)
    console.log(' - signing string');
    console.log(signing_string)
  }
  console.log(' - signing string');
  console.log(signing_string)
  // TODO add datetime window auth logic

  // get the key details from the backend and authorize the request
	pg.getApiKeyById(key_id, function (errors, apiKey) {
		if (errors) {
      res.status(403)
      return res.json({error:errors})
		} else {
      const signing_key = getSigningKey(apiKey.key, key_datestamp, apiKey.name, key_scope)
      const request_signature = hmac(signing_key, signing_string, 'hex')

      console.log(3)
      // add the key into the request
      req.scope = apiKey.scope;
      
      console.log('signing_key hex')
      console.log(Buffer.from(signing_key, 'utf8').toString('hex'))
      console.log('request_signature')
      console.log(request_signature)
      console.log('client_signature')
      console.log(client_signature)
      
      console.log(3)
      if (request_signature !== client_signature) {
        res.status(403)
        return res.json({error:errors})
      } else {
        next()
      }
		}
  })
}

function getSigningKey(key_secret, key_datestamp, key_name, key_scope) {
  var kDate = hmac("LT" + key_secret, key_datestamp);
  var kName = hmac(kDate, key_name);
  var kScope = hmac(kName, key_scope);
  var kSigning = hmac(kScope, "lt_request");
  return kSigning;
}

function hmac(key, string, encoding) {
  return crypto.createHmac('sha256', key).update(string, 'utf8').digest(encoding)
}

function hash(string, encoding) {
  return crypto.createHash('sha256').update(string, 'utf8').digest(encoding)
}

/*
 * Helper function to check if a value exists.
 * Provide table name and value;
 */
function valueExists(table, key, value, callback) {
	pg.checkTableForValue(table, key, value, function (reply) {
		if (reply) {
			callback(true);
		} else {
			callback(false);
		}
	});
}

/*
 * Function to test if a user exists.
 * On success will proceed to add a new user and relay
 * the status of 'addNewUser' to the callback
 */
function addVerifiedNewUser(username, email, password, defaultview, callback) {
	async.parallel({
		u: function(callback_) {
			valueExists("users", "username", username,
				function(response) {
					callback_(null,response);
				})
		},
		e: function(callback_) {
			valueExists("users", "email", email,
				function(response) {
					callback_(null,response);
				})
		}
	}, function(err, results) {
		if ((results.u === true) || (results.e === true)) {
			console.log("user already exists");
			callback("already_exists");
		} else if ((results.u === false) && (results.e === false)) {
			console.log("proceed to add new user");
			addNewUser(username, email, password, defaultview, "", "non-facebook-user",
				function(response) {
					callback(response);
				});
		} else {
			callback("error something went wrong testing if user exists");
		}
	});
}

/*
 * Function to add a new user.
 * On success callback will receive the new user id.
 */
function addNewUser(username, email, password, defaultview, facebook_user_id, facebookinfo, callback) {
	var account_status;
	var confirm_code;
	// regular accounts require email verification.
	//	account_status = "unconfirmed";
	//	confirm_code = genRandStr();
		account_status = "confirmed";
		confirm_code = "";
	// assign a default color to the user
	var colors = ['#1258c7','#560393','#a30051','#f40c26','#fe8100','#ffc300','#2fb601','#acd322','#bfdc4c','#34b5ec','#fed93a','#f00b7a','#fea0d0','#fb4e9b','#8e76da','#7659cc','#5129b1','#00c6bc','#86e2df'];
	var color_code = colors[Math.floor(Math.random()*colors.length)];
	let enc_data = JSON.stringify(aesgcm.encrypt(password));
	pg.addUser(username, email, enc_data, defaultview, facebook_user_id, facebookinfo, account_status, 'individual', confirm_code, color_code, 'user', function(e,r) {
		if (e) {
			console.log("err " + e);
			callback(e);
		} else {
			callback(r);
		}
	});
}


/*
 * Endpoint and control to test if user/email exists.
 * Used for registration form validation.
 */
exports.checkUserEmail = function(req, res) {
	var queryData = url.parse(req.url, true).query;
	if (queryData.username) {
		valueExists("users", "username", queryData.username,
			function(response) {
				if (response === false) {
					res.setHeader("Content-Type", "text/json");
					res.setHeader("Access-Control-Allow-Origin", "*");
					res.send({"username":"available"});
				} else {
					res.setHeader("Content-Type", "text/json");
					res.setHeader("Access-Control-Allow-Origin", "*");
					res.send({"username":"exists"});
				}
			});
	} else if (queryData.email) {
		valueExists("users", "email", queryData.email,
			function(response) {
				if (response === false) {
					res.setHeader("Content-Type", "text/json");
					res.setHeader("Access-Control-Allow-Origin", "*");
					res.send({"email":"available"});
				} else {
					res.setHeader("Content-Type", "text/json");
					res.setHeader("Access-Control-Allow-Origin", "*");
					res.send({"email":"exists"});
				}
			});
	}
}

/*
 * Endoint and control for user login.
 * Accepts email and password and if present attempt login.
 */
exports.login = function(req, res, next) {
  console.log(req.body);
	if (req.body.email && req.body.password) {
		valueExists("users", "email", req.body.email, function(response) {
			if (response === false) {
				res.status(200);
				res.setHeader("Content-Type", "text/json");
				res.send({"login":"failed"});
			} else {
				pg.getUserInfo(req.body.email, ['id','password','account_status'], function(e, userinfo) {
                    if (e) {
                        res.status(200);
                        res.setHeader("Content-Type", "text/json");
                        res.send({"login":"failed"});
                        return;
                    }
					var password_data;
					try {
						password_data = JSON.parse(userinfo.password);
					} catch (e) {
						console.log(e);
						res.status(200);
						res.json({"login":"failed"});
					}
					if (Array.isArray(password_data)) {
						if (password_data.length === 3) {
							// return authenticate(userinfo.id, req, res);
							if (aesgcm.decrypt(password_data) === req.body.password) {
								if (userinfo.account_status === "confirmed") {
									console.log("login succeeded");
									authenticate(userinfo.id, req, res);
								} else {
									res.status(200);
									res.json({"login":"failed"});
								}
							} else {
								res.status(200);
								res.json({"login":"failed"});
							}
						} else {
							res.status(200);
							res.json({"login":"failed"});
						}
					} else {
						res.status(200);
						res.json({"login":"failed"});
					}
				});
			}
		});
	}
}


/*
 * Endpoint and control for user registration.
 */
exports.register = function(req, res, next) {
	// make sure user is not already registered,
	// is not spam, has filled in information correctly.
	console.log('register attempt');
	var email;
	var username;
	var password;
	var language;
	if (req.body.email && req.body.password) {
		email = req.body.email;
		username = req.body.email;
		password = req.body.password;
		language = req.body.language;
	} else {
		res.status(400);
		res.send("Invalid register request.");
		return;
	}
	if (!language) {
		language = "en";
	}
	var defaultview = "student";
	if (req.body.teacher === "on") {
		defaultview = "teacher";
	}
	addVerifiedNewUser(username, email, password, defaultview, function(response) {
		// the response is the new user id
		if (response === parseInt(response, 10)) {
			// if the response looks valid then send the confirmation email.
			// the confirmation email needs send the code for addNewUser so retrieve that first.
			pg.getUserInfo(email, ['confirm_code'], function(e, userinfo) {
				if (userinfo.confirm_code) {
					var url = 'https://edivy.com/confirmregistration?' + 
						'email=' + email + '&code=' + userinfo.confirm_code;
					switch(language) {
						case "en":
							var mail = {
								from: 'noreply@bb.com',
								to: email,
								subject: 'Confirm your new account at edivy.com',
								html: '<html>' +
								'Thank your for Registering at Edivy.com.' +
								'To confirm your account please access the following link.' +
								'<br>' +
								'<a href="' + url + '">' + url + '</a>' +
								'<br>' +
								'Sent from Edivy' +
								'</html>'
							}
							break;
						case "ja":
							var mail = {
								from: 'noreply@edivy.com',
								to: email,
								subject: 'Edivy.comでの新しいアカウントの確認',
								html: '<html>' +
								'Edivy.comへのご登録ありがとうございます。' +
								'アカウントを確認するには以下のリンクへアクセスしてください。' +
								'<br>' +
								'<a href="' + url + '">' + url + '</a>' +
								'<br>' +
								'Edivyから' +
								'</html>'
							}
							break;
					}
                    pg.addToMailQueue(email, JSON.stringify(mail), function(e,r) {
						if (e) {
							console.log(e);
							console.log("user added but unable to add email to mail queue");
							res.status('401');
							res.json({"status":"failed"});
							return;
						} else {
							res.status('200');
							res.json({"status":"success"});
							return;
						}
					});
				} else {
					console.log("user added but unable to send email");
					res.status('401');
					res.json({"status":"failed"});
					return;
				}
			});
		} else if (response === "already_exists") {
			res.status('401');
			res.json({"status":"exists"});
			return;
		} else {
			console.log("unable to add new USER");
			res.status('401');
			res.json({"status":"failed"});
			return;
		}
	});
}

/*
 * Endpoint to request reset a user password
 */
exports.forgotPassword = function(req, res, next) {
	var queryData = url.parse(req.url, true).query;
	console.log('reset user password attempt');
	var email = queryData.email;
	var language = queryData.language;
	if (!email) {
		res.status(400);
		res.send("Invalid reset password request.");
		return;
	}
	if (!language) {
		language = "en";
	}
	var confirm_code = genRandStr();
	pg.resetConfirmCode(email, confirm_code, function(response) {
		var email_ = email;
		if (response === true) {
			pg.getUserInfo(email_, ['confirm_code'], function(e, userinfo) {
				if (userinfo) {
					if (userinfo.confirm_code) {
						var url = 'https://edivy.com/?action=resetpassword&' + 
							'email=' + email_ + '&code=' + userinfo.confirm_code;
						switch(language) {
							case "en":
								mail = {
									from: 'noreply@edivy.com',
									to: email_,
									subject: 'Reset password request for account at edivy.com',
									html: '<html>' +
									'Hi' +
									'<br>' +
									'If you made this request please access the following link to reset your password.' +
									'<br>' +
									'<a href="' + url + '">' + url + '</a>' +
									'<br>' +
									'If you did NOT request a new password, ignore this email and your password will remain unchanged.' +
									'Sent from Edivy' +
									'Questions? Contact us at <a href="mailto:support@edivy.com">support@edivy.com</a>' +
									'</html>'
								}
								break;
							case "ja":
								mail = {
									from: 'noreply@edivy.com',
									to: email_,
									subject: 'Edivy.comのアカウントパスワードをレセットする',
									html: '<html>' +
									'こんにちは' +
									'<br>' +
									'我々はアカウントパスワード変更のリクエストを受け取りました。' +
									'もしリクエストをしたのであれば以下のリンクへアクセスしてパスワードのリセットを行なってください。' +
									'<br>' +
									'<a href="' + url + '">' + url + '</a>' +
									'<br>' +
									'もしリクエストをした覚えがない場合はこのメールを無視してください。なお、パスワードは変更されません。' +
									'Edivyから' +
									'質問がございましたら <a href="mailto:support@edivy.com">support@edivy.com</a>へご連絡ください。' +
									'</html>'
								}
								break;
						}
						pg.addToMailQueue(email_, JSON.stringify(mail), function(e,r) {
							if (e) {
								console.log(e);
								res.status('401');
								res.json({"status":"failed","message":e});
								return;
							} else {
								res.status('200');
								res.json({"status":"success"});
								return;
							}
						});
					} else {
						console.log("unable to reset password");
						res.status('200');
						res.json({"status":"failed"});
						return;
					}
				} else {
					console.log("unable to reset password");
					res.status('200');
					res.json({"status":"failed"});
					return;
				}
			});
		} else {
			console.log("unable to reset password");
			res.status('200');
			res.json({"status":"failed"});
			return;
		}
	});
}

/*
 * Endpoint to reset a user password
 */
exports.resetPassword = function(req, res, next) {
	var queryData = url.parse(req.url, true).query;
	console.log('reset user password attempt');
	var email = queryData.email;
	var password= queryData.password;
	var confirm_code = queryData.confirm_code;
	if (!(email&&password&&confirm_code)) {
		res.status(200);
		res.json({"status":"failed"});
		return;
	}
	if (password.length<6) {
		res.status(200);
		res.json({"status":"failed"});
		return;
	}
	let enc_data = JSON.stringify(aesgcm.encrypt(password));
	pg.resetPassword(email, enc_data, confirm_code, function(response) {
		if (response === true) {
			res.status('200');
			res.json({"status":"success"});
			return;
		} else {
			console.log("unable to reset password");
			res.status('200');
			res.json({"status":"failed"});
			return;
		}
	});
}

/*
 * Endpoint confirming email (last step for user registration).
 */
exports.confirmEmail = function(req, res, next) {
	var queryData = url.parse(req.url, true).query;
	var code = queryData.code;
	var email = queryData.email;

	if (!email && !code) {
		console.log("missing email and/or code");
		res.status(401);
		res.json({
			error: "failed to confirm registration"
		});
		return;
	}
	pg.getUserInfo(email, ['id', 'account_status', 'confirm_code'], function(e, userinfo) {
		var res_ = res;
        if (e) {
            console.log("err " + e);
            res_.status(200);
            res_.json({
                status: "failed"
            });
            return;
		} else if (userinfo.confirm_code === code) {
            pg.confirmUser(userinfo.id, function(e, r) {
                if (e) {
                    console.log("err " + e);
                    res_.status(200);
                    res_.json({
                        status: "failed"
                    });
                } else {
                    res_.status(200);
                    res_.json({
                        status: "success"
                    });
                }
            });
        } else {
            res_.status(200);
            res_.json({
                status: "failed"
            });
        }
	});
}


exports.logout = function(req,res,next) {
  console.log(req.user_id)
  console.log(req.client_id)
	if (!req.user_id && !req.client_id) {
		res.send(503);
		return;
	}
	var user_clients;
	if (req.user_id in userClients) {
		// remove from db
		if (!isEmptyObject(userClients[req.user_id])) {
			// TODO delete user client
			pg.removeUserClient(userClients[req.user_id][req.client_id], function(e, r) {
				if (e) {
					console.log("unable to set user clients " + user_clients);
					console.log(e);
					res.sendStatus(503);
				} else {
          // remove the active object
          delete userClients[req.user_id][req.client_id];
          // logout this users cookie session.
          console.log("user id " + req.user_id + " loggedout");
          res.cookie("petcookie", "", { expires: new Date() });
          res.status(200);
          res.json({
              authorized: false,
              message: "logged out"
          });
        }
        return;
			});
		} else {
			res.status(200);
			res.json({
				authorized: false,
				message: "logged out"
			});
			return;
		}
	}
}


/*
 * Function to get the username from a user id.
 */
exports.getNameFromId = function(req, res, next) {
	var queryData = url.parse(req.url, true).query;
	var user_id;
	if (typeof queryData.user_id === 'undefined' || !queryData.user_id) {
		user_id = req.user_id;
	} else if (Number.isInteger(parseInt(queryData.user_id))) {
		user_id = queryData.user_id;
	} else {
		res.status(200);
		res.send("invalid request");
		return;
	}
    pg.getUsernameFromId(user_id, function(e,r) {
		if (e) {
			res.status(404);
			res.send({"error":"error in user exists (get users)"});
		} else {
			res.status(200);
            if (r) {
                res.send({"username":r});
            } else {
                res.send({"username":""});
            }
		}
	});
}

exports._getNameFromId = function(user_id, callback) {
	if (!Number.isInteger(parseInt(user_id))) {
        callback();
	} else {
        pg.getUsernameFromId(user_id, function(e, u) {
            if (e) {
                callback(e, null);
            } else {
                callback(null, u);
            }
        });
    }
}

/*
 * Given a list of user ids, send a list of ids with names.
 */
exports.getUsersList = function(req, res, next, list) {
    pg.getUsernamesFromIds(list, function(e,r) {
		if (e) {
            console.log(e);
            res.json({"status":"failed","error":e});
		} else {
            res.json(r);
        }
    });
}

/*
 * Function to get the authenticated status for a user.
 */
exports.getUserInfo = function(req, res, next) {
	if (req.user_id) {
		pg.getUserInfo(req.user_id, ['default_view', 'username', 'color_code', 'email'], function(e, userinfo) {
			if (e) {
				res.status(200);
                res.json({
					user_id: req.user_id,
					clientid: req.client_id,
					authorized: true,
                    defaultview: "guest",
					language: req.language,
					region: req.region,
					message: "token verified"
                });
			} else {
				res.status(200);
				res.json({
					user_id: req.user_id,
					clientid: req.client_id,
					authorized: true,
					defaultview: userinfo.default_view,
					language: req.language,
					region: req.region,
					username: userinfo.username,
					email: userinfo.email,
					color_code: userinfo.color_code,
					message: "token verified"
				});
			}
		});
	} else {
		res.status(200);
		res.json({
			user_id:"",
			authorized: false,
			message: "not authorized",
			selectedview: "",
			language: req.language,
			region: req.region,
			defaultview: "guest"
		});
	}
	return;
}

exports.getMailQueueLength = function(callback) {
    pg.getMailQueueLength(function(e,r) {
		if (e) {
			console.log("error ? " +e);
            callback(0);
		} else if (r) {
			callback(r);
        } else {
            callback(0);
        }
    });
}

exports.getFromMailQueue = function(callback) {
    pg.getFromMailQueue(function(e,r) {
		if (e) {
			console.log("error ? " +err);
            callback();
		} else if (r) {
			callback(r);
		} else {
            callback();
        }
    });
}

exports.setMailQueueStatus = function(email, status) {
    pg.setMailQueueStatus(email, status, function(e,r) {
		if (e) {
			console.log("error ? " +err);
            callback();
        }
    });
}

/*
 * set user email
 */
exports.setUserMail = function(req, res, next) {
	var queryData = url.parse(req.url, true).query;
	var email = queryData.email;
    pg.setUserMail(req.user_id, email, function(e,r) {
		if (e) {
            console.log(e);
			res.json({"email":"exists"});
		} else {
			res.status(200);
			res.json({"email":"success"});
        }
    });
}

exports.saveAccountDetails = function(req, res) {
	console.log("save account details req.body");
	console.log(JSON.stringify(req.body));
	var data = req.body;
	var bank_name = data.bank_name;
	var branch_name = data.branch_name;
	var holder_address_1 = data.holder_address_1;
	var holder_address_2  = data.holder_address_2;
	var holder_city = data.holder_city;
	var holder_country = data.holder_country;
	var holder_name = data.holder_name;
	var holder_zip_code = data.holder_zip_code;
	var number = data.number;
	var sort_code = data.sort_code;
	var type = data.type;
    pg.saveAccount(req.user_id, bank_name, branch_name, holder_address_1, holder_address_2, holder_city, holder_country, holder_name, holder_zip_code, number, sort_code, type, function(e,r) {
		if (e) {
            console.log(e);
			res.json({"email":"exists"});
		} else {
			res.status(200);
			res.json({"email":"success"});
        }
    });
}

exports.getAccountDetails = function(req, res) {
    pg.getAccount(req.user_id, function(e,account) {
		if (e) {
			res.status(503);
			res.json({"status":"failed"});
		} else {
		  res.json({
			  account
		  });
		}
	});
}

function isEmptyObject(obj) {
  for (var key in obj) {
    if (Object.prototype.hasOwnProperty.call(obj, key)) {
      return false;
    }
  }
  return true;
}

