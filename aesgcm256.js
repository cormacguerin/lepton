const buffer = require('buffer');
const crypto = require('crypto');

const aes256gcm = (key) => {
  const ALGO = 'aes-256-gcm';

  const encrypt = (str) => {
	const iv = Buffer.from(crypto.randomBytes(16), 'utf8');
	const cipher = crypto.createCipheriv(ALGO, key, iv);

	let enc = cipher.update(str, 'utf8', 'base64');
	enc += cipher.final('base64');
	return [enc, iv, cipher.getAuthTag()];
  };

  const decrypt = (enc, iv, authTag) => {
  // console.log("enc " + enc);
  // console.log("iv " + iv);
  // console.log("at" + authTag);

	const decipher = crypto.createDecipheriv(ALGO, key, iv);
	decipher.setAuthTag(authTag);
	let str = decipher.update(enc, 'base64', 'utf8');
	str += decipher.final('utf8');
	return str;
  };

  return {
	encrypt,
	decrypt,
  };
};

const KEY = new Buffer.from([173, 79, 191, 73, 255, 36, 130, 151, 227, 12, 177, 224, 9, 198, 190, 174, 173, 79, 191, 73, 255, 36, 130, 151, 227, 12, 177, 224, 9, 198, 190, 174], 'utf8');

const aesCipher = aes256gcm(KEY);

exports.encrypt = function(input) {
  return aesCipher.encrypt(input);
}

exports.decrypt = function(ed) {
  var d = '';
  try {
	d = aesCipher.decrypt(ed[0], Buffer.from(ed[1].data), Buffer.from(ed[2].data));
  } catch (e) {
	console.log(e);
  }
  return d;
} 

