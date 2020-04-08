#include "index_manager.h"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include "base64.h"
#include <algorithm>
#include <math.h>
#include <pqxx/strconv.hxx>
#include "frag.h"
#include "util.h"


using namespace std;
using namespace pqxx;


IndexManager::IndexManager(Frag::Type u, Frag::Type b, Frag::Type t, std::string db, std::string tb, std::string cl, std::string df) {
//    unigramFragManager(u,db,tb), bigramFragManager(b,db,tb), trigramFragManager(t,db,tb) {
  for (std::vector<std::string>::iterator lit = langs.begin(); lit != langs.end(); lit++) {
    std::cout << "*lit " << *lit << std::endl;
    std::cout << "db " << db << std::endl;
    std::cout << "tb " << tb << std::endl;
    unigramFragManager[*lit] = new FragManager(u,db,tb,*lit);
    bigramFragManager[*lit] = new FragManager(b,db,tb,*lit);
    trigramFragManager[*lit] = new FragManager(t,db,tb,*lit);
  }
  database = db;
  table = tb;
  columns = cl;
  display_field = df;
}

IndexManager::~IndexManager()
{
  for (std::vector<std::string>::iterator lit = langs.begin(); lit != langs.end(); lit++) {
    delete unigramFragManager[*lit];
    delete bigramFragManager[*lit];
    delete trigramFragManager[*lit];
  }
}

void IndexManager::init(std::string database) {

	strcpy(SPS, "\xe2\x96\x81");
	SPC = "~`!@#$%^&*()_-+=|\\}]{[\"':;?/>.<, ";
	// char SPC = {'~', '`', '!', '@', '#' , '$', '%', '^', '&', '*', '(', ')', '_', '+', '|', '\\', '{', '}', ':', '"', '|', '<', '>', '?', '/', '.', ',', '\'', ';', ']', '[', '-', '='};
	// spp.init();
	seg.init(database);

	// this is a redis connection (were replacing this with postgres for the index)
	// client.connect();
	// postgres connection
	try {
		C = new pqxx::connection("dbname = \'" + database + "\' user = postgres password = kPwFWfYAsyRGZ6IomXLCypWqbmyAbK+gnKIW437QLjw= hostaddr = 127.0.0.1 port = 5432");
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

void IndexManager::processFeeds() {

  std::set<std::string> run_langs;
  std::map<std::string, int> num_docs;
	int max_doc_id;
	int batch_size;
	int base_batch_size = 10000;
	getNumDocs(num_docs);
	getMaxDocId(max_doc_id);
	std::cout << "indexManager.cc : total num_docs : " << num_docs["total"] << std::endl;
	std::cout << "indexManager.cc : max_doc_id : " << max_doc_id << std::endl;
	int num_batches = num_docs["total"]/base_batch_size;
	if (num_batches < 1) {
		num_batches = 1;
		batch_size = max_doc_id;
	} else {
		batch_size = max_doc_id/num_batches;
	}
	std::cout << "indexManager.cc : batch_size " << batch_size << std::endl;
	std::cout << "indexManager.cc : num_batches : " << num_batches << std::endl;
	std::string statement = "SELECT lt_id,url,concat(" + columns + ") as document,language FROM \"" + table + "\" WHERE lt_id BETWEEN $1 AND $2";

	int batch_position = 0;

  std::vector<int> ids;
	for (int i = 0; i <= max_doc_id; ) {
		batch_position += batch_size;

		pqxx::work txn(*C);
		C->prepare("process_docs_batch", statement);

		std::cout << "indexManager.cc : process documents for batch " << i << " " << batch_position << " " << table << std::endl;
		pqxx::result r = txn.prepared("process_docs_batch")(i)(batch_position).exec();
		txn.commit();

		for (pqxx::result::const_iterator row = r.begin(); row != r.end(); ++row) {
			const pqxx::field id = (row)[0];
			const pqxx::field url = (row)[1];
			const pqxx::field document = (row)[2];
			const pqxx::field lang = (row)[3];

			std::cout << "url : " << url.c_str() << "(" << lang << ")" << std::endl;
			if (url.is_null()) {
				std::cout << "skip : url is null" << std::endl;;
				continue;
			}
			if (document.is_null()) {
				std::cout << "skip : document is null" << std::endl;;
				continue;
			}
			if (lang.is_null()) {
				std::cout << "skip : lang is null" << std::endl;;
				continue;
			}
      if (std::find(langs.begin(), langs.end(), lang.c_str()) == langs.end()) {
        continue;
      }
      run_langs.insert(lang.c_str());
			indexDocument(id.c_str(), url.c_str(), document.c_str(), lang.c_str());
      ids.push_back(std::stoi(id.c_str()));
		}
		i = batch_position;
    // we need a corpus to determine ranking score, as the corpus changes
    // ranking may get skewed, eg initially indexed documents may have inaccurate
    // ranking, as such reindexing is important to ensure normalization.
    // merge fragments
    for (std::set<std::string>::iterator lit = run_langs.begin(); lit != run_langs.end(); lit++) {
      unigramFragManager.at(*lit)->mergeFrags(num_docs.at(*lit), database);
      bigramFragManager.at(*lit)->mergeFrags(num_docs.at(*lit), database);
      trigramFragManager.at(*lit)->mergeFrags(num_docs.at(*lit), database);
    }
    // process doc info must be called after mergeFrage(which creates idf)
//    processDocInfo(ids);
    ids.clear();
	}
	// sync the remainder.
	std::cout << "indexManager.cc : batch finished - sync remaining terms." << std::endl;
  for (std::set<std::string>::iterator lit = run_langs.begin(); lit != run_langs.end(); lit++) {
    unigramFragManager.at(*lit)->syncFrags();
    bigramFragManager.at(*lit)->syncFrags();
    trigramFragManager.at(*lit)->syncFrags();
    // merge frag fragments into frag.
    // atually just running one merges all, todo, split it up.
    unigramFragManager.at(*lit)->mergeFrags(num_docs.at(*lit), database);
    bigramFragManager.at(*lit)->mergeFrags(num_docs.at(*lit), database);
    trigramFragManager.at(*lit)->mergeFrags(num_docs.at(*lit), database);
  }
  // process doc info must be called after mergeFrage(which creates idf)
//  processDocInfo(ids);
}

/*
 * This is our second pass, here we set the docscore.
 * docscore = product(num_terms*term_idf)/sum(terms)
 * We can't do this in the frst pass above as the IDF
 * for each term is unknown. The best trade off is to process
 * in batches however again Since the score is based on the
 * corpus we need to reindex to assist in score normalization.
 * We also do basic entity extraction here, we
 * could do other processing too eg. ML stuff etc
 */
void IndexManager::processDocInfo(std::vector<int> batch) {

	// this statement calculates the idf
	std::cout << "index_manager.cc processDocInfo" << std::endl;
	
	std::vector<std::string> unibitri{"trigrams","bigrams","unigrams"};

	std::string update_doc_entities = "UPDATE \"" + table + "\" SET lt_entities = $1 WHERE lt_id = $2";

	pqxx::work txn(*C);

	float multiplier = 3.0;
	for (std::vector<std::string>::const_iterator it = unibitri.begin(); it != unibitri.end(); it++) {

    std::string update_docscore = "WITH v AS (WITH d AS (SELECT docterms.key, max(array_length(regexp_split_to_array(docterms.value, ','), 1)), language FROM \"" + table + "\" d, jsonb_each_text(d.lt_segmented_grams->'"+*it+"') docterms WHERE d.lt_id = $1 GROUP BY docterms.key, language) SELECT DISTINCT (SUM(d.max) OVER()) AS freq, (SUM(d.max * t.idf) OVER()) AS score FROM d INNER JOIN lt_"+*it+" AS t ON d.key = t.gram WHERE d.language = t.lang GROUP BY d.max, t.idf) UPDATE \"" + table + "\" SET lt_docscore = (lt_docscore + (SELECT score/freq FROM v))/2 WHERE lt_id = $1";
    C->prepare("process_" + *it + "_docscore", update_docscore);

	  std::vector<std::pair<std::string,float>> grams;
    
    for (std::vector<int>::iterator it_ = batch.begin(); it_ != batch.end(); it_++) {

      // update the docuscore using the idf / term frequencies
      pqxx::result rds = txn.prepared("process_" + *it + "_docscore")(*it_).exec();

      // basic entity extraction function.
      std::string gram_terms = "SELECT d.lt_id, key, (CHAR_LENGTH(value) - CHAR_LENGTH(REPLACE(value, ',', ''))), lt_" + *it + ".idf AS i FROM \"" + table + "\" d, jsonb_each_text(d.lt_segmented_grams->'"+*it+"') docterms INNER JOIN lt_"+*it+" ON docterms.key = lt_"+*it+".gram WHERE d.lt_id = $1 ORDER BY i DESC LIMIT 30";
      C->prepare("process_"+*it+"_batch", gram_terms);
      pqxx::result rgt = txn.prepared("process_"+*it+"_batch")(*it_).exec();
//     std::cout << " - statement - " << gram_terms << std::endl;
//     std::cout << " - value - " << *it_ << std::endl;

      for (pqxx::result::const_iterator row = rgt.begin(); row != rgt.end(); ++row) {
        const pqxx::field gram = (row)[1];
        const pqxx::field weight = (row)[2];
//        std::cout << " - gram " << gram.c_str() << std::endl;
//        std::cout << " - weight " << weight.c_str() << std::endl;
        std::string t = std::string(gram.c_str());
        float w = atof(weight.c_str()) * multiplier;
        bool add = true;
        if (hasDigit(t)==false) {
          std::cout << t << " - " << w << std::endl;
          if (grams.empty()) {
            grams.push_back(std::pair<std::string,float>(t,w));
          } else if (grams.back().second < w) {
            for (int i=0; i < grams.size(); i++) {
              if (grams.at(i).second < w) {
                grams.insert(grams.begin()+i, std::pair<std::string,float>(t,w));
                if (grams.size() > 20) {
                  grams.pop_back();
                }
                break;
              }
            }
          }
          /*
          if (add==true) {
            grams.push_back(std::pair<std::string,float>(t,w));
          }
          */
        }
      }
      std::sort(grams.begin(), grams.end(), [](auto &left, auto &right) {
        return left.second < right.second;
      });
      // This is very innefficient, we should use reserve at the beingging and just insert the lower values
      std::string strarray = "";
      for (std::vector<std::pair<std::string,float>>::iterator pit_ = grams.begin(); pit_ != grams.end(); pit_++) {
        strarray += pit_->first;
        if (std::next(pit_) != grams.end()) {
          strarray += ",";
        }
      }
      grams.clear();
      std::cout << *it_ <<  " : " << strarray << std::endl;
      C->prepare("process_doc_entities", update_doc_entities);
      pqxx::result ent = txn.prepared("process_doc_entities")(strarray)(*it_).exec();
    }
    multiplier=multiplier-1.0;
	}
}

/*
 * For each entry in the docfeeds table we ..
 * - read the data 
 * - parse the json
 * - base64 decode the encoded contents.
 * - segment the body
 */
void IndexManager::indexDocument(string id, string pkey, string rawdoc, std::string lang) {
	// create main json doc and load rawdoc into it.
  /*
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
	// const string display_url = doc[display_field].GetString();
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
  */
	
	// container for our url term / frequency
	std::map<std::string, Frag::Item> doc_unigram_map;
	std::map<std::string, Frag::Item> doc_bigram_map;
	std::map<std::string, Frag::Item> doc_trigram_map;
	// seg.parse(id, display_field, lang, decoded_doc_body, 
	seg.parse(id, pkey, lang, rawdoc, table, display_field,
		doc_unigram_map, doc_bigram_map, doc_trigram_map);
	unigramFragManager[lang]->addTerms(doc_unigram_map);
	bigramFragManager[lang]->addTerms(doc_bigram_map);
	trigramFragManager[lang]->addTerms(doc_trigram_map);
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
	C->prepare("export_vocab", "SELECT gram, incidence FROM ngrams WHERE lang = $1 AND gram NOT LIKE '% %' ORDER BY gram");
	pqxx::work txn(*C);
	pqxx::result r = txn.prepared("export_vocab")(lang).exec();
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

void IndexManager::getNumDocs(std::map<std::string, int> &count) {
	prepare_doc_count(*C);
	pqxx::work txn(*C);
	pqxx::result r = txn.prepared("doc_count").exec();
	txn.commit();
  int total = 0;
	for (pqxx::result::const_iterator row = r.begin(); row != r.end(); ++row) {
		const pqxx::field l = (row)[0];
		const pqxx::field c = (row)[1];
    count[l.c_str()] = atoi(c.c_str());
    total += atoi(c.c_str());
  }
  count["total"] = total;
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

void IndexManager::getMaxDocId(int &num) {
	prepare_max_doc_id(*C);
	pqxx::work txn(*C);
	pqxx::result r = txn.prepared("max_doc_id").exec();
	txn.commit();
	const pqxx::field c = r.back()[0];
	num = atoi(c.c_str());
}

std::vector<int> IndexManager::GetDocscoreBatch() {
	prepare_docscore_batch(*C);
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

void IndexManager::prepare_docscore_batch(pqxx::connection_base &c) {
	c.prepare("docscore_batch", "SELECT lt_id, lang FROM \"" + table + "\" WHERE lt_segmented_grams IS NOT NULL AND entities IS NULL LIMIT 10");
}

void IndexManager::prepare_max_doc_id(pqxx::connection_base &c) {
	c.prepare("max_doc_id", "SELECT MAX(lt_id) FROM \"" + table +"\"");
}

void IndexManager::prepare_max_unigram_id(pqxx::connection_base &c, std::string lang) {
	c.prepare("max_unigram_id", "SELECT MAX(id) FROM unigrams WHERE lang = " + lang);
}

void IndexManager::prepare_max_bigram_id(pqxx::connection_base &c, std::string lang) {
	c.prepare("max_bigram_id", "SELECT MAX(id) FROM bigrams WHERE lang = " + lang);
}

void IndexManager::prepare_max_trigram_id(pqxx::connection_base &c, std::string lang) {
	c.prepare("max_trigram_id", "SELECT MAX(id) FROM trigrams WHERE lang = " + lang);
}

void IndexManager::prepare_doc_count(pqxx::connection_base &c) {
  c.prepare("doc_count", "SELECT language, COUNT(language) AS count FROM \"" + table + "\" GROUP BY language");
}

void IndexManager::prepare_unigram_count(pqxx::connection_base &c, std::string lang) {
	c.prepare("unigram_count", "SELECT COUNT(*) FROM unigrams WHERE lang = " + lang);
}

void IndexManager::prepare_bigram_count(pqxx::connection_base &c, std::string lang) {
	c.prepare("bigram_count", "SELECT COUNT(*) FROM bigrams WHERE lang = " + lang);
}

void IndexManager::prepare_trigram_count(pqxx::connection_base &c, std::string lang) {
	c.prepare("trigram_count", "SELECT COUNT(*) FROM trigrams WHERE lang = " + lang);
}

bool IndexManager::hasDigit(const std::string& s)
{
	return std::any_of(s.begin(), s.end(), ::isdigit);
}

