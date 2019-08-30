#include "proton.h"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include "base64.h"
#include <algorithm>


using namespace std;
using namespace pqxx;


Proton::Proton()
{
}

Proton::~Proton()
{
}

void Proton::init() {
	strcpy(SPS, "\xe2\x96\x81");
	SPC = "~`!@#$%^&*()_-+=|\\}]{[\"':;?/>.<, ";
	//char SPC = {'~', '`', '!', '@', '#' , '$', '%', '^', '&', '*', '(', ')', '_', '+', '|', '\\', '{', '}', ':', '"', '|', '<', '>', '?', '/', '.', ',', '\'', ';', ']', '[', '-', '='};
	// spp.init();
	seg.init();

	// this is a redis connection (were replacing this with postgres for the index)
	//	client.connect();
	// postgres connection
	try {
		C = new pqxx::connection("dbname = index user = postgres password = kPwFWfYAsyRGZ6IomXLCypWqbmyAbK+gnKIW437QLjw= hostaddr = 127.0.0.1 port = 5432");
    	if (C->is_open()) {
    	   cout << "Opened database successfully: " << C->dbname() << endl;
    	} else {
    	   cout << "Can't open database" << endl;
    	}
	} catch (const std::exception &e) {
		cerr << e.what() << std::endl;
		exit;
	}
}

void Proton::processFeeds(std::string lang) {
	cout << "process feeds for " << lang << endl;

	//C->prepare("process", "SELECT * FROM docs ORDER BY index_date NULLS FIRST LIMIT $1");
	C->prepare("process", "SELECT * FROM docs WHERE index_date is NULL");
	pqxx::work txn(*C);
	//pqxx::result r = txn.prepared("process")("22018").exec();
	pqxx::result r = txn.prepared("process").exec();
	txn.commit();

	for (pqxx::result::const_iterator row = r.begin(); row != r.end(); ++row) {
//		for (pqxx::row::const_iterator field = row->begin(); field != row->end(); ++field) {
//			std::cout << field->c_str() << std::endl;
//		}
		const pqxx::field id = (row)[0];
		const pqxx::field url = (row)[1];
		const pqxx::field feed = (row)[2];
		std::cout << " - - - - - " << std::endl;
		std::cout << "url : " << url.c_str() << std::endl;
		std::cout << "lang : " << lang.c_str() << std::endl;
		if (url.is_null()) {
			std::cout << "skip : url is null" << std::endl;;
			continue;
		}
		if (feed.is_null()) {
			std::cout << "skip : feed is null" << std::endl;;
			continue;
		}
		if (lang.empty()) {
			std::cout << "skip : lang is null" << std::endl;;
			continue;
		}
	
		indexDocument(id.c_str(), url.c_str(), feed.c_str(), lang);
	}

}


/*
 * For each entry in the docfeeds table we ..
 * - read the data 
 * - parse the json
 * - base64 decode the encoded contents.
 * - segment the body
 */
void Proton::indexDocument(string id, string dockey, string rawdoc, string lang) {
	// create main json doc and load rawdoc into it.
	rapidjson::Document doc;
	const char *cstr = rawdoc.c_str();
	try { 
		doc.Parse(cstr);
	} catch (const exception& e) {
		cout << "Error : Aborting due to failed JSON parse attempt" << endl;
		cout << "Error Message : " << e.what() << endl;
		return;
	}
	// const char *ckey = (*it).c_str();
	const string display_url = doc["display_url"].GetString();
	string doc_body;
	try {
		doc_body = doc["body"].GetString();
	} catch (const exception& e) {
		cout << "Warning : unable to parse display_url " << e.what() << endl;
	}
	// base64 decode
	string decoded_doc_body = base64_decode(doc_body);
	// tokenize
	vector<string> tokenized_doc_body;
	// this is the sentencepiece tokenizer
	// spp.tokenize(decoded_doc_body, &tokenized_doc_body);
	// this is the cormac tokenizer
	seg.parse(id, dockey, lang, decoded_doc_body);
}

bool Proton::isSPS(char firstchar) {
	if (firstchar == *SPS) {
	//	std::cout << " firstchar matches " << std::endl;
		return true;
	} else {
	//	std::cout << " firstchar doesn't match " << std::endl;
		return false;
	}
}


void Proton::exportVocab(std::string lang) {
	C->prepare("export_vocab", "select gram, incidence from ngrams WHERE gram NOT LIKE '% %' ORDER BY gram");
	pqxx::work txn(*C);
	pqxx::result r = txn.prepared("export_vocab").exec();
	txn.commit();

	for (pqxx::result::const_iterator row = r.begin(); row != r.end(); ++row) {
		const pqxx::field gram = (row)[0];
		const pqxx::field incidence = (row)[1];
		std::cout << gram.c_str() << " " << incidence.c_str() << std::endl;
	}

	/*
	cout << "export vocab for " << lang << endl;

	vector<string> vocabfeeds;

	//client.smembers("vocabfeeds", [&vocabfeeds](cpp_redis::reply& reply) {
	client.smembers("doc_id_" +lang, [&vocabfeeds](cpp_redis::reply& reply) {
		for (auto k: reply.as_array()) {
			vocabfeeds.push_back(k.as_string());
		}
	});

	client.sync_commit();

	ofstream crawled_contents ("crawled_contents_" +lang+ ".txt");
	for(vector<string>::iterator it = vocabfeeds.begin(); it != vocabfeeds.end(); ++it) {
		string bodytext;
		client.hget("doc_feed_" +lang, *it, [it, &bodytext](cpp_redis::reply& reply) {
			rapidjson::Document vocab;
			const char *cstr = reply.as_string().c_str();
			try {
				vocab.Parse(cstr);
			} catch (const exception& e) {
				cout << "Error : Aborting due to failed JSON parse attempt" << endl;
				cout << "Error Message : " << e.what() << endl;
				return;
			}
			string vocab_body;
			try {
				vocab_body = vocab["body"].GetString();
			} catch (const exception& e) {
				cout << "Warning : unable to parse display_url " << e.what() << endl;
			}
			bodytext = base64_decode(vocab_body);
		});
		client.sync_commit();
		crawled_contents << bodytext;
	}
	crawled_contents.close();
	*/
}


