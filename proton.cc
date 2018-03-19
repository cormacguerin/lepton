#include "proton.h"
#include <iostream>
#include <fstream>
#include <cpp_redis/cpp_redis>
#include <unistd.h>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "base64.h"
#include <algorithm>


using namespace std;

cpp_redis::client client;


Proton::Proton()
{
}

Proton::~Proton()
{
}

void Proton::init() {
	SPS = "\xe2\x96\x81";
	SPC = "~`!@#$%^&*()_-+=|\\}]{[\"':;?/>.<, ";
	//char SPC = {'~', '`', '!', '@', '#' , '$', '%', '^', '&', '*', '(', ')', '_', '+', '|', '\\', '{', '}', ':', '"', '|', '<', '>', '?', '/', '.', ',', '\'', ';', ']', '[', '-', '='};
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

	string feed;
	for(vector<string>::iterator it = docfeeds.begin(); it != docfeeds.end(); ++it) {
		feed = "";
		client.hget("doc_feed", *it, [it, &feed](cpp_redis::reply& reply) {
			if (reply != NULL) {
				feed = reply.as_string();
			}
		});
		client.sync_commit();
		if (feed != "") {
			std::cout << "start processing for " << *it << std::endl;
			indexDocument(*it, feed);
		}
	}
}


/*
 * For each entry in the docfeeds table we ..
 * - read the data 
 * - parse the json
 * - base64 decode the encoded contents.
 * - segment the body
 */
void Proton::indexDocument(string dockey, string rawdoc) {
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
	spp.tokenize(decoded_doc_body, &tokenized_doc_body);
	
	// string to acumulate the text into one doc.
	string bodytext = "";
	// map for counting word occurrences.
	std::map<std::string, int> seg_weights;
	for(std::vector<std::string>::iterator it = tokenized_doc_body.begin(); it != tokenized_doc_body.end(); ++it) {
		// concat unparsed(original) body text
		bodytext = bodytext + *it;
		// convert to lowercase
		std::transform((*it).begin(), (*it).end(), (*it).begin(), ::tolower);
		// remove sentencepiece leading space
		if (isSPS((*it).at(0))) {
			(*it).erase(0, 3);
		}
		// remove stop words
		// normalize stems
		// remove special characters.
		for (int i=0; i < sizeof(SPC); i++) {
			const char *a = (*it).c_str();
			for (int j=(*it).size(); j>=0; j--) {
				if (a[j] == SPC[i]) {
					/*
					cout << "*it " << *it << endl;
					cout << "a[j] " << a[j] << endl;
					cout << "SPC[i] " << SPC[i] << endl;
					cout << "erase at j = " << j << endl;
					*/
					(*it).erase(j, 1);
					/*
					cout << "*it " << *it << endl;
					cout << " - - -  " << endl;
					*/
				}
			}
		}
		std::vector<string> key;
		key.push_back(dockey);
		client.sadd((*it).c_str(), key, [](cpp_redis::reply& reply) {
		});
		// add a weight for this term.
		if (seg_weights.count(*it) == 1) {
			seg_weights.at(*it) = seg_weights.at(*it)+1;
		} else {
			seg_weights.insert(std::pair<string,int>(*it,1));
		}
	}
	for (map<string, int>::iterator m = seg_weights.begin(); m != seg_weights.end(); ++m) {
		cout << m->first << endl;
		cout << m->second << endl;
	}
	client.sync_commit();
	cout << bodytext << endl;
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

