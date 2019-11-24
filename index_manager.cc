#include "index_manager.h"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include "base64.h"
#include <algorithm>
#include <math.h>
#include <pqxx/strconv.hxx>
#include "frag.h"


using namespace std;
using namespace pqxx;


IndexManager::IndexManager(Frag::Type u, Frag::Type b, Frag::Type t) : unigramFragManager(u), bigramFragManager(b), trigramFragManager(t) {
}

IndexManager::~IndexManager()
{
}

void IndexManager::init() {


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

void IndexManager::processFeeds(std::string lang) {
	int num_docs;
	int max_doc_id;
	int batch_size;
	int base_batch_size = 10000;
	getNumDocs(num_docs, lang);
	getMaxDocId(max_doc_id, lang);
	std::cout << "indexManager.cc : num_docs : " << num_docs << std::endl;
	std::cout << "indexManager.cc : max_doc_id : " << max_doc_id << std::endl;
	int num_batches = num_docs/base_batch_size;
	if (num_batches < 1) {
		num_batches = 1;
		batch_size = max_doc_id;
	} else {
		batch_size = max_doc_id/num_batches;
	}
	std::cout << "indexManager.cc : batch_size " << batch_size << std::endl;
	std::cout << "indexManager.cc : num_batches : " << num_batches << std::endl;
	std::string statement = "SELECT id,url,feed FROM docs_" + lang + " WHERE id BETWEEN $1 AND $2";

	int batch_position = 0;

	for (int i = 0; i <= max_doc_id; ) {
		batch_position += batch_size;

		pqxx::work txn(*C);
		C->prepare("process_docs_batch", statement);

		std::cout << "indexManager.cc : process feeds for batch " << i << " " << batch_position << " " << lang << std::endl;
		pqxx::result r = txn.prepared("process_docs_batch")(i)(batch_position).exec();
		txn.commit();

		for (pqxx::result::const_iterator row = r.begin(); row != r.end(); ++row) {
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
		i = batch_position;
	}
	// sync the remainder.
	std::cout << "indexManager.cc : batch finished - sync remaining terms." << std::endl;
	unigramFragManager.syncFrags();
	bigramFragManager.syncFrags();
	trigramFragManager.syncFrags();
	// merge frag fragments into frag.
	// atually just running one merges all, todo, split it up.
	unigramFragManager.mergeFrags(num_docs, lang);
	bigramFragManager.mergeFrags(num_docs, lang);
	trigramFragManager.mergeFrags(num_docs, lang);
}

/*
 * This is our second pass, here we set the docscore.
 * docscore = product(num_terms*term_idf)/sum(terms)
 * We can't do this in the frst pass above as the IDF
 * for each term is unknown. The score is based on the 
 * corpus so we need to index the corpus first.
 * We could do other processing here eg. ML stuff etc
 */
void IndexManager::processDocInfo(std::string lang) {

	// this statement calculates the idf
	std::cout << "index_manager.cc processDocInfo" << std::endl;
	
	std::vector<std::string> unibitri{"trigrams","bigrams","unigrams"};

	std::string update_doc_idf = "WITH v AS (WITH d AS (SELECT docterms.key, max(array_length(regexp_split_to_array(docterms.value, ','), 1)) FROM docs_en d, jsonb_each_text(d.segmented_grams->'unigrams') docterms WHERE d.id = $1 GROUP BY docterms.key) SELECT DISTINCT (SUM(d.max) OVER()) AS freq, (SUM(d.max * t.idf) OVER()) AS score FROM d INNER JOIN unigrams_en AS t ON d.key = t.gram GROUP BY d.max, t.idf) UPDATE docs_en SET docscore = (SELECT score/freq FROM v) WHERE id = $1";

	std::string update_doc_topics = "UPDATE docs_en SET topics = $1 WHERE id = $2";

	std::vector<int> b = GetDocscoreBatch(lang);

	while (b.size() > 0) {
		std::cout << "b.size() " << b.size()  << std::endl;
		pqxx::work txn(*C);
		for (std::vector<int>::iterator it_ = b.begin(); it_ != b.end(); it_++) {
			C->prepare("process_docscore_batch", update_doc_idf);
			C->prepare("process_topics_batch", update_doc_topics);
			pqxx::result rds = txn.prepared("process_docscore_batch")(*it_).exec();

			std::vector<std::pair<std::string,float>> grams;
			std::map<std::string,std::vector<std::pair<std::string,float>>> unigrams;
			float multiplier = 3.0;
			for (std::vector<std::string>::const_iterator it = unibitri.begin(); it != unibitri.end(); it++) {
				std::string gram_terms = "SELECT d.id, key, (CHAR_LENGTH(value)- CHAR_LENGTH(REPLACE(value, ',', '')))*" + *it + "_en.idf AS i FROM docs_en d, jsonb_each_text(d.segmented_grams->'"+*it+"') docterms INNER JOIN "+*it+"_en ON docterms.key="+*it+"_en.gram WHERE d.id = $1 ORDER BY i DESC LIMIT 30";
				C->prepare("process_"+*it+"_batch", gram_terms);
				pqxx::result rgt = txn.prepared("process_"+*it+"_batch")(*it_).exec();
				for (pqxx::result::const_iterator row = rgt.begin(); row != rgt.end(); ++row) {
					const pqxx::field gram = (row)[1];
					const pqxx::field weight = (row)[2];
					std::string t = std::string(gram.c_str());
					float w = atof(weight.c_str())*multiplier;
					bool add = true;
					if (hasDigit(t)==false) {
						std::cout << t << " - " << w << std::endl;
						if (grams.empty()) {
							grams.push_back(std::pair<std::string,float>(t,w));
						}
						for (std::vector<std::pair<std::string,float>>::iterator git = grams.begin(); git != grams.end(); git++) {
							if (git->first.find(t) != std::string::npos) {
								if (w < git->second) {
									add=false;
									std::cout << " - keep " << git->first << "(" << git->second << ") over " << t << "(" << w << ")" << std::endl;
									break;
								} else {
									std::cout << " - swap " << git->first << "(" << git->second << ") for " << t << "(" << w << ")" << std::endl;
									grams.erase(git--);
								}
							}
						}
						if (add==true) {
							grams.push_back(std::pair<std::string,float>(t,w));
						}
					}
				}
				multiplier=multiplier-1.0;
			}
			std::sort(grams.begin(), grams.end(), [](auto &left, auto &right) {
				return left.second < right.second;
			});
			std::string strarray = "";
			for (std::vector<std::pair<std::string,float>>::iterator pit_ = grams.begin(); pit_ != grams.end(); pit_++) {
				//std::cout << pit_->first << "  -  " << pit_->second << std::endl;
				strarray += pit_->first;
				if (std::next(pit_) != grams.end()) {
					strarray += ",";
				}
			}
			std::cout << *it_ <<  " : " << strarray << std::endl;
			pqxx::result rgt = txn.prepared("process_topics_batch")(strarray)(*it_).exec();
		}
		txn.commit();
		b.clear();
		b = GetDocscoreBatch(lang);
	}
}

/*
 * For each entry in the docfeeds table we ..
 * - read the data 
 * - parse the json
 * - base64 decode the encoded contents.
 * - segment the body
 */
void IndexManager::indexDocument(string id, string dockey, string rawdoc, string lang) {
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
	std::vector<string> tokenized_doc_body;
	// this is the sentencepiece tokenizer
	// spp.tokenize(decoded_doc_body, &tokenized_doc_body);
	// this is the cormac tokenizer
	
	// container for our url term / frequency
	std::map<std::string, Frag::Item> doc_unigram_map;
	std::map<std::string, Frag::Item> doc_bigram_map;
	std::map<std::string, Frag::Item> doc_trigram_map;
	seg.parse(id, dockey, lang, decoded_doc_body, 
		doc_unigram_map, doc_bigram_map, doc_trigram_map);
	unigramFragManager.addTerms(doc_unigram_map);
	bigramFragManager.addTerms(doc_bigram_map);
	trigramFragManager.addTerms(doc_trigram_map);
}

bool IndexManager::isSPS(char firstchar) {
	if (firstchar == *SPS) {
	//	std::cout << " firstchar matches " << std::endl;
		return true;
	} else {
	//	std::cout << " firstchar doesn't match " << std::endl;
		return false;
	}
}

void IndexManager::exportVocab(std::string lang) {
	C->prepare("export_vocab", "select gram, incidence from ngrams WHERE gram NOT LIKE '% %' ORDER BY gram");
	pqxx::work txn(*C);
	pqxx::result r = txn.prepared("export_vocab").exec();
	txn.commit();

	for (pqxx::result::const_iterator row = r.begin(); row != r.end(); ++row) {
		const pqxx::field gram = (row)[0];
		const pqxx::field incidence = (row)[1];
		std::cout << gram.c_str() << " " << incidence.c_str() << std::endl;
	}

}

/*
 * This is all very messy but it works.
 */

void IndexManager::getNumDocs(int &count, std::string lang) {
	prepare_doc_count(*C, lang);
	pqxx::work txn(*C);
	pqxx::result r = txn.prepared("doc_count").exec();
	txn.commit();
	const pqxx::field c = r.back()[0];
	count = atoi(c.c_str());
}

void IndexManager::getNumNgrams(int &count, std::string gram, std::string lang) {
	if (gram=="uni") {
		prepare_unigram_count(*C, lang);
	} else if (gram=="bi") {
		prepare_bigram_count(*C, lang);
	} else if (gram=="tri") {
		prepare_trigram_count(*C, lang);
	} else {
		return;
	}
	pqxx::work txn(*C);
	pqxx::result r = txn.prepared(gram+"gram_count").exec();
	txn.commit();
	const pqxx::field c = r.back()[0];
	count = atoi(c.c_str());
}

void IndexManager::getMaxNgramId(int &num, std::string gram, std::string lang) {
	if (gram=="uni") {
		prepare_max_unigram_id(*C, lang);
	} else if (gram=="bi") {
		prepare_max_bigram_id(*C, lang);
	} else if (gram=="tri") {
		prepare_max_trigram_id(*C, lang);
	} else {
		return;
	}
	pqxx::work txn(*C);
	pqxx::result r = txn.prepared("max_"+gram+"gram_id").exec();
	txn.commit();
	const pqxx::field c = r.back()[0];
	num = atoi(c.c_str());
}


void IndexManager::getMaxDocId(int &num, std::string lang) {
	prepare_max_doc_id(*C, lang);
	pqxx::work txn(*C);
	pqxx::result r = txn.prepared("max_doc_id").exec();
	txn.commit();
	const pqxx::field c = r.back()[0];
	num = atoi(c.c_str());
}

std::vector<int> IndexManager::GetDocscoreBatch(std::string lang) {
	prepare_docscore_batch(*C, lang);
	pqxx::work txn(*C);
	pqxx::result r = txn.prepared("docscore_batch").exec();
	txn.commit();

	std::vector<int> b;
	for (pqxx::result::const_iterator row = r.begin(); row != r.end(); ++row) {
		const pqxx::field c = (row)[0];
		b.push_back(atoi(c.c_str()));
	}
	return b;
}

void IndexManager::prepare_docscore_batch(pqxx::connection_base &c, std::string lang) {
	c.prepare("docscore_batch", "SELECT id FROM docs_" + lang + " WHERE segmented_grams IS NOT NULL AND topics IS NULL LIMIT 1000");
}

void IndexManager::prepare_max_doc_id(pqxx::connection_base &c, std::string lang) {
	c.prepare("max_doc_id", "SELECT MAX(id) FROM docs_" + lang);
}

void IndexManager::prepare_max_unigram_id(pqxx::connection_base &c, std::string lang) {
	c.prepare("max_unigram_id", "SELECT MAX(id) FROM unigrams_" + lang);
}

void IndexManager::prepare_max_bigram_id(pqxx::connection_base &c, std::string lang) {
	c.prepare("max_bigram_id", "SELECT MAX(id) FROM bigrams_" + lang);
}

void IndexManager::prepare_max_trigram_id(pqxx::connection_base &c, std::string lang) {
	c.prepare("max_trigram_id", "SELECT MAX(id) FROM trigrams_" + lang);
}

void IndexManager::prepare_doc_count(pqxx::connection_base &c, std::string lang) {
	c.prepare("doc_count", "SELECT COUNT(*) FROM docs_" + lang);
}

void IndexManager::prepare_unigram_count(pqxx::connection_base &c, std::string lang) {
	c.prepare("unigram_count", "SELECT COUNT(*) FROM unigrams_" + lang);
}

void IndexManager::prepare_bigram_count(pqxx::connection_base &c, std::string lang) {
	c.prepare("bigram_count", "SELECT COUNT(*) FROM bigrams_" + lang);
}

void IndexManager::prepare_trigram_count(pqxx::connection_base &c, std::string lang) {
	c.prepare("trigram_count", "SELECT COUNT(*) FROM trigrams_" + lang);
}

bool IndexManager::hasDigit(const std::string& s)
{
	return std::any_of(s.begin(), s.end(), ::isdigit);
}

