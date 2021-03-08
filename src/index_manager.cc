
/*
 *  &copy; Cormac Guerin , Invoke Network
 *  Copyright 2021 All Rights Reserved.
 *
 *  All information contained herein is, and remains
 *  the property of Cormac Guerin & Invoke Network
 *
 *  The intellectual and technical concepts within remain
 *  the sole property of the aforementioned owners.
 *
 *  Reproduction and / or distriptions of this software is 
 *  strictly prohibited.
 */


#include "index_manager.h"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <thread>
#include "base64.h"
#include <algorithm>
#include <math.h>
#include <pqxx/strconv.hxx>
#include "frag.h"
#include "util.h"
// #include "score_document.h"


using namespace std;
using namespace pqxx;

static auto config = getConfig();

IndexManager::IndexManager(Frag::Type u, Frag::Type b, Frag::Type t, std::string db, std::string tb, std::string cl) {
    do_run = true;
    //    unigramFragManager(u,db,tb), bigramFragManager(b,db,tb), trigramFragManager(t,db,tb) {
    for (std::vector<std::string>::iterator lit = langs.begin(); lit != langs.end(); lit++) {
        unigramFragManager[*lit] = new FragManager(u,config.postgres_database,tb,*lit);
        bigramFragManager[*lit] = new FragManager(b,config.postgres_database,tb,*lit);
        trigramFragManager[*lit] = new FragManager(t,config.postgres_database,tb,*lit);
    }
    database = config.postgres_database;
    table = tb;
    columns = cl;
    init();
}

IndexManager::~IndexManager()
{
    for (std::vector<std::string>::iterator lit = langs.begin(); lit != langs.end(); lit++) {
        delete unigramFragManager[*lit];
        delete bigramFragManager[*lit];
        delete trigramFragManager[*lit];
    }
    C->disconnect();
    delete C;
}

void IndexManager::init() {

    strcpy(SPS, "\xe2\x96\x81");
    SPC = "~`!@#$%^&*()_-+=|\\}]{[\"':;?/>.<, ";
    // char SPC = {'~', '`', '!', '@', '#' , '$', '%', '^', '&', '*', '(', ')', '_', '+', '|', '\\', '{', '}', ':', '"', '|', '<', '>', '?', '/', '.', ',', '\'', ';', ']', '[', '-', '='};
    // spp.init();
    seg.init(database);

    // this is a redis connection (were replacing this with postgres for the index)
    // client.connect();
    // postgres connection
    try {
		    C = new pqxx::connection("dbname = " + config.postgres_database + " user = " + config.postgres_user + " password = " + config.postgres_password + " hostaddr = " + config.postgres_host + " port = " + config.postgres_port);
        if (C->is_open()) {
            cout << "Opened database successfully: " << C->dbname() << endl;
        } else {
            cout << "Can't open database" << endl;
        }
    } catch (const std::exception &e) {
        cerr << e.what() << std::endl;
        exit(1);
    }
}

void IndexManager::spawnProcessFeeds() {
    std::cout << "index_manager.cc : " << database << " " << table << "  spawnProcessFeeds" << std::endl;
    processFeeds();
}

void IndexManager::processFeeds() {

    // start the merge frags thread if it's not already running
    if (merge_frags == false) {
        std::thread t(runFragMerge, this);
        t.detach();
    }

    while (do_run) {

        std::set<std::string> run_langs;
        int max_doc_id;
        int batch_size = 0;
        int base_batch_size = 50000;
        getMaxDocId(max_doc_id);

        std::vector<int> ids;

        std::string statement = "SELECT id,url,concat(" + columns + ") as document,lang FROM \"" + table + "\" WHERE (feed_date > index_date OR index_date IS null) LIMIT $1";

        pqxx::work txn(*C);
        C->prepare("process_docs_batch", statement);

        pqxx::result r = txn.prepared("process_docs_batch")(base_batch_size).exec();
        txn.commit();

        // std::cout << "index_manager.cc : " << database << " " << table << "  index " << columns << " for " << " " << r.size() << " docs." << std::endl;

        int counter;
        int skipped_docs = 0;
        for (pqxx::result::const_iterator row = r.begin(); row != r.end(); ++row) {
            counter++;
            const pqxx::field id = (row)[0];
            const pqxx::field url = (row)[1];
            const pqxx::field document = (row)[2];
            const pqxx::field lang = (row)[3];

            // std::cout << "url : " << url.c_str() << "(" << lang << ")" << std::endl;
            if (url.is_null()) {
                // std::cout << "skip : url is null" << std::endl;
                skipped_docs++;
                continue;
            }
            if (document.is_null()) {
                // std::cout << "skip : document is null" << std::endl;
                skipped_docs++;
                continue;
            }
            if (lang.is_null()) {
                // std::cout << "skip : lang is null" << std::endl;
                skipped_docs++;
                continue;
            }
            if (std::find(langs.begin(), langs.end(), lang.c_str()) == langs.end()) {
                skipped_docs++;
                continue;
            }
            run_langs.insert(lang.c_str());
            indexDocument(id.c_str(), document.c_str(), lang.c_str());
            ids.push_back(std::stoi(id.c_str()));

            // process suggestions that begin with stop words, this is really a hack
            // we should relpace with ML suggestions
			if (counter == base_batch_size-1) {
                for (std::set<std::string>::iterator lit = run_langs.begin(); lit != run_langs.end(); lit++) {
                    updateStopSuggest(*lit, (int)r.size());
                }
            }
        }

        // i = batch_position;
        // process doc info must be called after mergeFrage(which creates idf)
        // because mergefrags is now running on a different thread constantly this shouldn't be an issue.
        ids.clear();

        // sync 
        for (std::set<std::string>::iterator lit = run_langs.begin(); lit != run_langs.end(); lit++) {
            unigramFragManager.at(*lit)->syncFrags();
            bigramFragManager.at(*lit)->syncFrags();
            trigramFragManager.at(*lit)->syncFrags();
            updateStopSuggest(*lit, (int)r.size());
        }
        // process doc info must be called after mergeFrags(which creates idf)
        //
        // sleep a few seconds, and try again
        usleep(6000000);
        std::cout << "processed " << counter << " docs skipped " << skipped_docs << std::endl;
    }
    std::cout << "finish process feeds" << std::endl;
    return;
}

// we need a corpus to determine ranking score, as the corpus changes
// ranking may get skewed, eg initially indexed documents may have inaccurate
// ranking, as such reindexing is important to ensure normalization.
// 
// Running this is a separate thread is problematic as it does large updates which block the database
// This ends up blocking the indexer so there is no real advantage to just running it in sequence.
//
// By running this in a separate thread we could constrantly have the ranking updated.
// The main loop also runs this after each batch but in a separate thread we could get near
// realtime indexing/serving
//
// I guess we will need to come back to this, how can we have non blocking updates.
// Perhaps run a separate database or table or use flat files...
void IndexManager::runFragMerge(IndexManager* indexManager) {

    std::cout << "index_manager.cc : " << indexManager->database << " " << indexManager->table << "  begin runFragMerge " << std::endl;

    std::map<std::string, int> num_docs;
    indexManager->getNumDocs(num_docs);
    indexManager->merge_frags = true;

    auto config = getConfig();

    pqxx::connection* C_;
    try {
		    C_ = new pqxx::connection("dbname = " + config.postgres_database + " user = " + config.postgres_user + " password = " + config.postgres_password + " hostaddr = " + config.postgres_host + " port = " + config.postgres_port);
        if (C_->is_open()) {
            cout << "Opened database successfully: " << C_->dbname() << endl;
        } else {
            cout << "Can't open database" << endl;
        }
    } catch (const std::exception &e) {
        cerr << e.what() << std::endl;
        exit(1);
    }

    while (indexManager->do_run) {
        
        std::map<int,std::string> purge_docs;
        std::string statement = "SELECT id, index_date FROM \"" + indexManager->table + "\" WHERE update = true";
        std::cout << "index_manager.cc : " << indexManager->database << " " << indexManager->table << " " << statement << std::endl;

        pqxx::work txn(*C_);
        C_->prepare("get_updated_docs", statement);

        pqxx::result r = txn.prepared("get_updated_docs").exec();

        for (pqxx::result::const_iterator row = r.begin(); row != r.end(); ++row) {
            const pqxx::field id = (row)[0];
            const pqxx::field date = (row)[1];
            if (date.is_null() == false ){
                purge_docs.insert(std::pair<int,std::string>(atoi(id.c_str()),std::string(date.c_str())));
            }
        }

        std::cout << "index_manager.cc : " << indexManager->database << " " << indexManager->table << " start runFragMerge with " << purge_docs.size() << " docs to purge" << std::endl;

        for (std::map<std::string, int>::iterator lit = num_docs.begin(); lit != num_docs.end(); lit++) {
            std::cout << "index_manager.cc : " << indexManager->database << " " << indexManager->table << " runFragMerge " << lit->first << std::endl;
            if (indexManager->unigramFragManager.find(lit->first) != indexManager->unigramFragManager.end()) {
                indexManager->unigramFragManager.at(lit->first)->mergeFrags(lit->second, indexManager->database, purge_docs);
            }
            if (indexManager->bigramFragManager.find(lit->first) != indexManager->bigramFragManager.end()) {
                indexManager->bigramFragManager.at(lit->first)->mergeFrags(lit->second, indexManager->database, purge_docs);
            }
            if (indexManager->trigramFragManager.find(lit->first) != indexManager->trigramFragManager.end()) {
                indexManager->trigramFragManager.at(lit->first)->mergeFrags(lit->second, indexManager->database, purge_docs);
            }
        }

        std::string statement_ = "UPDATE \"" + indexManager->table + "\" SET update = false WHERE id = $1 AND index_date = $2";
        C_->prepare("update_purged_docs", statement_);
        // std::cout << "index_manager.cc " << indexManager->database << " " << indexManager->table << " " << statement_ << std::endl;

        std::cout << statement_ << std::endl;
        for (std::map<int,std::string>::const_iterator ii = purge_docs.begin(); ii != purge_docs.end(); ii++) {
            std::cout << "index_manager.cc " << indexManager->database << " " << indexManager->table << " " << ii->first << " " << ii->second << std::endl;
            pqxx::result r = txn.prepared("update_purged_docs")(ii->first)(ii->second).exec();
        }

        txn.commit();
        std::cout << "index_manager.cc : " << indexManager->database << " " << indexManager->table << " end runFragMerge with " << purge_docs.size() << " docs to purge" << std::endl;
        
        // running process_doc info here slows things down a lot.
        std::vector<int> empty;
        //ScoreDocument scoreDoc;
        processDocInfo(empty,indexManager->database,indexManager->table,config.postgres_password);
        //std::cout << "runFragMerge done " << std::endl;
        usleep(60000000);
    }

    delete C_;
    std::cout << "runFragMerge " << indexManager->database << " " << indexManager->table << " end " << std::endl;
    indexManager->merge_frags = false;
    return;
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
void IndexManager::processDocInfo(std::vector<int> batch, std::string database, std::string table, std::string pwd) {
    std::cout << "index_manager.cc : " << database << " " << table << "  processDocInfo " << std::endl;

    if (batch.empty()) {
        try {
		        pqxx::connection C__("dbname = " + config.postgres_database + " user = " + config.postgres_user + " password = " + config.postgres_password + " hostaddr = " + config.postgres_host + " port = " + config.postgres_port);
            if (C__.is_open()) {
                std::cout << "Opened database successfully: " << C__.dbname() << std::endl;
                pqxx::work txn__(C__);
                C__.prepare("docs_to_score", "SELECT id FROM \"" + table + "\" WHERE index_date IS NOT null AND docscore IS null");
                pqxx::result r = txn__.prepared("docs_to_score").exec();
                txn__.commit();

                for (pqxx::result::const_iterator row = r.begin(); row != r.end(); ++row) {
                    batch.push_back(atoi((row)[0].c_str()));
                }
                C__.disconnect();
            } else {
                std::cout << "Can't open database" << std::endl;
            }
        } catch (const std::exception &e) {
            std::cerr << e.what() << std::endl;
        }
    }

    std::cout << "index_manager.cc : " << database << " " << table << "  processDocInfo batch size " << batch.size() << std::endl;

    try {
		    pqxx::connection C_("dbname = " + config.postgres_database + " user = " + config.postgres_user + " password = " + config.postgres_password + " hostaddr = " + config.postgres_host + " port = " + config.postgres_port);
        if (C_.is_open()) {
            // since this can be a thread so we need a new connection for each call.
            pqxx::work txn_(C_);

            // this statement calculates the idf

            // std::vector<std::string> unibitri{"trigrams","bigrams","unigrams"};
            std::vector<std::string> unibitri{"trigrams","bigrams","unigrams"};

            std::string update_doc_entities = "UPDATE \"" + table + "\" SET entities = $1 WHERE id = $2";

            std::vector<std::pair<std::string,float>> grams;

            for (std::vector<int>::iterator it_ = batch.begin(); it_ != batch.end(); it_++) {

                float multiplier = 1.0;

                for (std::vector<std::string>::const_iterator it = unibitri.begin(); it != unibitri.end(); it++) {

                    std::string update_docscore = "WITH v AS (WITH d AS (SELECT docterms.key, max(array_length(regexp_split_to_array(docterms.value, ','), 1)), lang FROM \"" + table + "\" d, jsonb_each_text(d.segmented_grams->'"+*it+"') docterms WHERE d.id = $1 GROUP BY docterms.key, lang) SELECT DISTINCT (SUM(d.max) OVER()) AS freq, (SUM(d.max * t.idf) OVER()) AS score FROM d INNER JOIN "+*it+" AS t ON d.key = t.gram WHERE d.lang = t.lang GROUP BY d.max, t.idf) UPDATE \"" + table + "\" SET docscore = (SELECT score/freq FROM v) WHERE id = $1";
                    C_.prepare("process_" + *it + "_docscore", update_docscore);

                    // update the docuscore using the idf / term frequencies
                    pqxx::result rds = txn_.prepared("process_" + *it + "_docscore")(*it_).exec();

                    // basic entity extraction function.
                    std::string gram_terms = "SELECT d.id, key, (CHAR_LENGTH(value) - CHAR_LENGTH(REPLACE(value, ',', ''))), " + *it + ".idf AS i FROM \"" + table + "\" d, jsonb_each_text(d.segmented_grams->'"+*it+"') docterms INNER JOIN "+*it+" ON docterms.key = "+*it+".gram WHERE d.id = $1 ORDER BY i DESC LIMIT 30";
                    C_.prepare("process_"+*it+"_batch", gram_terms);
                    pqxx::result rgt = txn_.prepared("process_"+*it+"_batch")(*it_).exec();

                    for (pqxx::result::const_iterator row = rgt.begin(); row != rgt.end(); ++row) {
                        const pqxx::field gram = (row)[1];
                        const pqxx::field weight = (row)[2];
                        const pqxx::field idf = (row)[3];
                        std::string t = std::string(gram.c_str());
                       // if (atof(idf.c_str()) < 1) {
                        //    continue;
                        //}
                        float w = (atof(weight.c_str()) * atof(idf.c_str())) * multiplier;
                        if (hasDigit(t)==false) {
                            if (grams.empty()) {
                                grams.push_back(std::pair<std::string,float>(t,w));
                            } else if (grams.back().second < w) {
                                for (int i=0; i < grams.size(); i++) {
                                    if (grams.at(i).second < w) {
                                        grams.insert(grams.begin()+i, std::pair<std::string,float>(t,w));
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    multiplier++;
                }
                std::sort(grams.begin(), grams.end(), [](auto &left, auto &right) {
                  return left.second < right.second;
                });
                grams.resize(20);
                // This is very innefficient, we should use reserve at the beingging and just insert the lower values
                std::string strarray = "";
                for (std::vector<std::pair<std::string,float>>::iterator pit_ = grams.begin(); pit_ != grams.end(); pit_++) {
                    if (pit_->first.length()>0) {
                        strarray += pit_->first;
                        // for debug
                        strarray += "(";
                        strarray += std::to_string(pit_->second);
                        strarray += ")";
                        if (std::next(pit_) != grams.end()) {
                            strarray += ",";
                        }
                    }
                }
                grams.clear();
                C_.prepare("process_doc_entities", update_doc_entities);
                pqxx::result ent = txn_.prepared("process_doc_entities")(strarray)(*it_).exec();
            }
            txn_.commit();
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }
}

/*
 * For each entry in the docfeeds table we ..
 * - read the data 
 * - parse the json
 * - base64 decode the encoded contents.
 * - segment the body
 */
void IndexManager::indexDocument(string id, string rawdoc, std::string lang) {
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
    seg.parse(id, lang, rawdoc, table,
            doc_unigram_map, doc_bigram_map, doc_trigram_map, stopSuggest[lang]);
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
        // std::cout << gram.c_str() << " " << incidence.c_str() << std::endl;
    }
}

/*
 * This is all very messy but it works.
 */

void IndexManager::getNumDocs(std::map<std::string, int> &count) {
    try {
		    pqxx::connection C__("dbname = " + config.postgres_database + " user = " + config.postgres_user + " password = " + config.postgres_password + " hostaddr = " + config.postgres_host + " port = " + config.postgres_port);
        if (C__.is_open()) {
            pqxx::work txn__(C__);
            prepare_doc_count(C__);
            pqxx::result r = txn__.prepared("doc_count").exec();
            txn__.commit();
            int total = 0;
            for (pqxx::result::const_iterator row = r.begin(); row != r.end(); ++row) {
                const pqxx::field l = (row)[0];
                const pqxx::field c = (row)[1];
                count[l.c_str()] = atoi(c.c_str());
                total += atoi(c.c_str());
            }
            C__.disconnect();
        } else {
            std::cout << "Can't open database" << std::endl;
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
    }

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

void IndexManager::updateStopSuggest(std::string lang, int batchsize) {
    prepare_update_stop_suggest(*C);
    pqxx::work txn(*C);

    for (std::map<std::vector<std::string>,double>::const_iterator it = stopSuggest[lang].begin(); it != stopSuggest[lang].end(); it++) {
        std::string gram= "";
        for (std::vector<std::string>::const_iterator i = it->first.begin(); i != it->first.end(); ++i) {
            gram += *i;
            if (std::next(i)!=it->first.end()) {
                gram += ":";
            }
        }
        // std::cout <<  "sugg " << s << " " << it->second << std::endl;
        pqxx::result r = txn.prepared("update_stop_suggest")(lang)(gram)(it->first.at(0))(it->second/batchsize).exec();
    }
    txn.commit();
}

void IndexManager::prepare_update_stop_suggest(pqxx::connection_base &c) {
    c.prepare("update_stop_suggest", "INSERT INTO stop_suggest (lang,gram,stop,idf) VALUES($1,$2,$3,$4) ON CONFLICT ON CONSTRAINT stop_suggest_lang_gram_key DO UPDATE SET idf = ((stop_suggest.idf + $4)/2) WHERE stop_suggest.lang = $1 AND stop_suggest.gram = $2");
}

void IndexManager::prepare_docscore_batch(pqxx::connection_base &c) {
    c.prepare("docscore_batch", "SELECT id, lang FROM \"" + table + "\" WHERE segmented_grams IS NOT NULL AND entities IS NULL LIMIT 10");
}

void IndexManager::prepare_max_doc_id(pqxx::connection_base &c) {
    c.prepare("max_doc_id", "SELECT MAX(id) FROM \"" + table +"\"");
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
    c.prepare("doc_count", "SELECT lang, COUNT(lang) AS count FROM \"" + table + "\" GROUP BY lang");
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

