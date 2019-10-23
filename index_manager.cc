#include "index_manager.h"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include "base64.h"
#include <algorithm>
#include <math.h>
#include <pqxx/strconv.hxx>


using namespace std;
using namespace pqxx;


IndexManager::IndexManager()
{
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
	fragManager.syncFrags();
	// merge frag fragments into frag.
	fragManager.mergeFrags(num_docs, lang);
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
	std::string statement = "WITH v AS (WITH d AS (SELECT docterms.key, max(array_length(regexp_split_to_array(docterms.value, ','), 1)) FROM docs_en d, jsonb_each_text(d.segmented_grams->'unigrams') docterms WHERE d.id=$1 GROUP BY docterms.key) SELECT d.max * t.idf AS m FROM d INNER JOIN unigrams_en AS t ON d.key = t.gram GROUP BY d.max, t.idf) UPDATE docs_en SET docscore = (SELECT SUM(m)/COUNT(m) FROM v) WHERE id=$1";

	std::vector<int> b = GetDocscoreBatch(lang);

	while (b.size() > 0) {
		std::cout << "b.size() " << b.size()  << std::endl;
		pqxx::work txn(*C);
		C->prepare("process_docscore_batch", statement);
		for (std::vector<int>::iterator it = b.begin(); it != b.end(); it++) {
			//std::cout << "id : " << *it << std::endl;
			pqxx::result r = txn.prepared("process_docscore_batch")(*it).exec();
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
	std::cout << "deb A" << std::endl;
	seg.parse(id, dockey, lang, decoded_doc_body, 
		doc_unigram_map, doc_bigram_map, doc_trigram_map);
	fragManager.addTerms(doc_unigram_map, doc_bigram_map, doc_trigram_map);
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

void IndexManager::updateNgramIdf(std::map<int, double> idfbatch, std::string gram, std::string lang) {
	if (gram=="uni") {
		prepare_update_unigram_idf(*C, lang);
	} else if (gram=="bi") {
		prepare_update_bigram_idf(*C, lang);
	} else if (gram=="tri") {
		prepare_update_trigram_idf(*C, lang);
	} else {
		return;
	}
	pqxx::work txn(*C);
	for (std::map<int, double>::iterator it = idfbatch.begin(); it != idfbatch.end(); it++) {
		pqxx::result r = txn.prepared("update_"+gram+"gram_idf")(it->second)(it->first).exec();
	}
	txn.commit();
}

/*
 * Run over all terms and populare the IDF(inverse document frequency) 
 * idf=log()
 */
void IndexManager::updateIdf(std::string lang) {
	//std::string ngrams[] = {"uni","bi","tri"};
	std::string ngrams[] = {"uni","bi","tri"};
	int num_docs;
	int num_ngrams;
	int max_ngram_id;
	for (const string &ng : ngrams) {
		int batch_position = 0;

		if (ng=="uni") {
			prepare_unigram_document_frequency(*C, lang);
		} else if (ng=="bi") {
			prepare_bigram_document_frequency(*C, lang);
		} else if (ng=="tri") {
			prepare_trigram_document_frequency(*C, lang);
		}
		std::cout << "indexManager.cc updateIdf processing for " << ng << "grams" << endl;;

		getNumDocs(num_docs, lang);
		getNumNgrams(num_ngrams, ng, lang);
		getMaxNgramId(max_ngram_id, ng, lang);
		std::cout << "num docs " << num_docs << std::endl;
		std::cout << num_ngrams << " " << ng << "grams " << " with max id of " << max_ngram_id << std::endl;
		if (num_ngrams == 0 || num_ngrams > max_ngram_id) {
			std::cout << "Aborting update idf, not enough " << ng << "grams." << std::endl;
			continue;
		}
		int batch_size = (max_ngram_id/num_ngrams)*1000;

		for (int i = 0; i < max_ngram_id; ) {
			batch_position += batch_size;
			pqxx::work txn(*C);
			std::cout << "db batch request started for between " << i << " and " << batch_position << std::endl;
			pqxx::result r = txn.prepared(ng+"gram_document_frequency")(i)(batch_position).exec();
			std::cout << "db batch request complete processing..";
			pqxx::result::const_iterator last_iter = r.end();
			last_iter--;
			std::string insert_value;
			std::map<int,double> idfbatch;
			for (pqxx::result::const_iterator row = r.begin(); row != r.end(); ++row) {
				const pqxx::field gram_id = (row)[0];
				const pqxx::field incidence = (row)[1];
				double idf = log((double)num_docs/incidence.as<double>());
				idfbatch.insert(std::pair<int,double>(gram_id.as<int>(),idf));
				std::string this_value = "{" + gram_id.as<std::string>() + "," + to_string(idf) + "}";
				insert_value += this_value;
				if (row != last_iter) {
					insert_value += ',';
				}
			}
			txn.commit();
			i = batch_position;
			// pqxx::result r_ = txn.prepared(ng+"gram_document_frequency")(i)(batch_position).exec();
			updateNgramIdf(idfbatch, ng, lang);
			int complete = (int)((double)batch_position/(double)max_ngram_id);
			if (complete > 1)
				complete = 1;
			std::cout << "Doc " << ng << "gram idf update " << complete*100 << "% complete" << std::endl;
		}
	}
}

void IndexManager::prepare_docscore_batch(pqxx::connection_base &c, std::string lang) {
	c.prepare("docscore_batch", "SELECT id FROM docs_" + lang + " WHERE docscore IS NULL LIMIT 1000");
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

void IndexManager::prepare_unigram_document_frequency(pqxx::connection_base &c, std::string lang) {
	c.prepare("unigram_document_frequency",
			"SELECT gram_id,count(url_id) FROM docunigrams_" + lang + " WHERE (SELECT gram_id BETWEEN $1 AND $2) GROUP BY gram_id");
}

void IndexManager::prepare_bigram_document_frequency(pqxx::connection_base &c, std::string lang) {
	c.prepare("bigram_document_frequency",
			"SELECT gram_id,count(url_id) FROM docbigrams_" + lang + " WHERE (SELECT gram_id BETWEEN $1 AND $2) GROUP BY gram_id");
}

void IndexManager::prepare_trigram_document_frequency(pqxx::connection_base &c, std::string lang) {
	c.prepare("trigram_document_frequency",
			"SELECT gram_id,count(url_id) FROM doctrigrams_" + lang + " WHERE (SELECT gram_id BETWEEN $1 AND $2) GROUP BY gram_id");
}

void IndexManager::prepare_update_unigram_idf(pqxx::connection_base &c, std::string lang) {
	c.prepare("update_unigram_idf",
		"WITH t as (UPDATE docunigrams_" + lang + " SET weight = ($1 * tf) WHERE gram_id = $2 RETURNING gram_id) "
		"UPDATE unigrams_" + lang + " SET idf = $1 WHERE id = $2");
}

void IndexManager::prepare_update_bigram_idf(pqxx::connection_base &c, std::string lang) {
	c.prepare("update_bigram_idf",
		"WITH t as (UPDATE docbigrams_" + lang + " SET weight = ($1 * tf) WHERE gram_id = $2 RETURNING gram_id) "
		"UPDATE bigrams_" + lang + " SET idf = $1 WHERE id = $2");
}

void IndexManager::prepare_update_trigram_idf(pqxx::connection_base &c, std::string lang) {
	c.prepare("update_trigram_idf",
		"WITH t as (UPDATE doctrigrams_" + lang + " SET weight = ($1 * tf) WHERE gram_id = $2 RETURNING gram_id) "
		"UPDATE trigrams_" + lang + " SET idf = $1 WHERE id = $2");
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

