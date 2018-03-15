#include "proton.h"
#include <iostream>
#include <fstream>
#include <cpp_redis/cpp_redis>
#include <unistd.h>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "base64.h"


using namespace std;

cpp_redis::client client;


Proton::Proton()
{
}

Proton::~Proton()
{
}

void Proton::init() {
	client.connect();
	spp.init();
}

void Proton::processFeeds() {
	cout << "processFeeds" << endl;

	vector<string> docfeeds;

	client.smembers("docfeeds", [&docfeeds](cpp_redis::reply& reply) {
			for (auto k: reply.as_array()) {
				docfeeds.push_back(k.as_string());
			}
	});

	client.sync_commit();
	/*
		for (map<string, string>::iterator mit = (*mechanics).begin(); mit != (*mechanics).end(); ++mit) {
			Render::Node* node = Render::NodeManager::getInstance().getNode(mit->first);
			cout << " MEHHHHH " << (mit->second).actors.size()  << endl;
			cout << mit->second.test.name << endl;
	*/

	string feed;
	for(vector<string>::iterator it = docfeeds.begin(); it != docfeeds.end(); ++it) {
		client.hget("content_feed", *it, [it, &feed](cpp_redis::reply& reply) {
			feed = reply.as_string();
		});
		client.sync_commit();
		indexDocument(feed);
	}
}


/*
 * For each entry in the docfeeds table we ..
 * - read the data 
 * - parse the json
 * - base64 decode the encoded contents.
 * - segment the body
 */
void Proton::indexDocument(string rawdoc) {
	rapidjson::Document doc;
	const char *cstr = rawdoc.c_str();
	try { 
		doc.Parse(cstr);
	} catch (const exception& e) {
		cout << "Error : Aborting due to failed JSON parse attempt" << endl;
		cout << "Error Message : " << e.what() << endl;
		return;
	}
	//		const char *ckey = (*it).c_str();
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
	cout << " debug " << endl;
	spp.tokenize(decoded_doc_body, &tokenized_doc_body);
	cout << " decoded_doc_body " << decoded_doc_body << endl;
	for(std::vector<std::string>::iterator it = tokenized_doc_body.begin(); it != tokenized_doc_body.end(); ++it) {
		std::cout << " parsed_contents  b " << *it << std::endl;
	}
//	cout << tokenized_doc_body << endl;
//		doc["body"].setString();
}


void Proton::processVocab() {
	cout << "processFeeds" << endl;

	vector<string> vocabfeeds;

	client.smembers("vocabfeeds", [&vocabfeeds](cpp_redis::reply& reply) {
		for (auto k: reply.as_array()) {
			vocabfeeds.push_back(k.as_string());
		}
	});

	client.sync_commit();

	ofstream rawvocab ("rawvocab.txt");
	for(vector<string>::iterator it = vocabfeeds.begin(); it != vocabfeeds.end(); ++it) {
		string bodytext;
		client.hget("content_feed", *it, [it, &bodytext](cpp_redis::reply& reply) {
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
		rawvocab << bodytext;
	}
	rawvocab.close();
}

