
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
    for (std::vector<std::string>::iterator lit = langs.begin(); lit != langs.end(); lit++) {
        unigramFragManager[*lit] = new FragManager(u,db,tb,*lit);
        bigramFragManager[*lit] = new FragManager(b,db,tb,*lit);
        trigramFragManager[*lit] = new FragManager(t,db,tb,*lit);
    }
    database = db;
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
    C->close();
    delete C;
    C_->close();
    delete C_;
}

void IndexManager::init() {

    strcpy(SPS, "\xe2\x96\x81");
    SPC = "~`!@#$%^&*()_-+=|\\}]{[\"':;?/>.<, ";
    // char SPC = {'~', '`', '!', '@', '#' , '$', '%', '^', '&', '*', '(', ')', '_', '+', '|', '\\', '{', '}', ':', '"', '|', '<', '>', '?', '/', '.', ',', '\'', ';', ']', '[', '-', '='};
    // spp.init();
    seg.init(database,table);

    // Indexiung process has two sections.
    // 1. C - connection for handling indexing of new and updated docs.
    // 2. C_ - connection for handling fragment/index merges
    //
    // We split these because frag merge and index doc purges and so on can be blocking
    // We don't want to slow down the indexing of documents so best to divide.
    // Currenctly we only have one indexing thread, thos should be increased eventually, in that case mutex needs to be added to 'C'
    try {
		    C = new pqxx::connection("dbname = " + database + " user = " + config.postgres_user + " password = " + config.postgres_password + " hostaddr = " + config.postgres_host + " port = " + config.postgres_port);
        if (C->is_open()) {
            cout << "Opened database successfully: " << C->dbname() << endl;
        } else {
            cout << "Can't open database" << endl;
        }
    } catch (const std::exception &e) {
        cerr << e.what() << std::endl;
        exit(1);
    }

    try {
		    C_ = new pqxx::connection("dbname = " + database + " user = " + config.postgres_user + " password = " + config.postgres_password + " hostaddr = " + config.postgres_host + " port = " + config.postgres_port);
        if (C_->is_open()) {
            cout << "Opened database successfully: " << C_->dbname() << endl;
        } else {
            cout << "Can't open database" << endl;
        }
    } catch (const std::exception &e) {
        cerr << e.what() << std::endl;
        exit(1);
    }

    prepare_max_doc_id();
    prepare_update_stop_suggest();
    prepare_docscore_batch();
    prepare_doc_count();
    prepare_get_docs();
    prepare_purge_docs();
    prepare_docs_batch();
    prepare_docs_to_score();
    prepare_process_doc_entites();
    for (std::vector<std::string>::const_iterator it = unibitri.begin(); it != unibitri.end(); it++) {
        prepare_update_docscore(*it);
        prepare_process_batch(*it);
    }
}

void IndexManager::spawnProcessFeeds() {
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

        pqxx::work txn(*C);
        pqxx::result r = txn.exec_prepared("process_docs_batch", base_batch_size);
        txn.commit();

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

    while (indexManager->do_run) {
        
        std::map<int,std::string> purge_docs;
        pqxx::result r = indexManager->getDocsToPurge();

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

        // std::cout << "index_manager.cc " << indexManager->database << " " << indexManager->table << " " << statement_ << std::endl;

        for (std::map<int,std::string>::const_iterator ii = purge_docs.begin(); ii != purge_docs.end(); ii++) {
            indexManager->purgeDocs(ii->first, ii->second);
        }

        std::cout << "index_manager.cc : " << indexManager->database << " " << indexManager->table << " end runFragMerge with " << purge_docs.size() << " docs to purge" << std::endl;
        
        // running process_doc info here slows things down a lot.
        std::vector<int> empty;
        // ScoreDocument scoreDoc;
        // processDocInfo MUST BE CALLED AFTER mergeFrags as idf calculation is needed
        processDocInfo(empty,indexManager->database,indexManager->table,config.postgres_password, indexManager);
        //std::cout << "runFragMerge done " << std::endl;
        usleep(60000000);
    }

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
void IndexManager::processDocInfo(std::vector<int> batch, std::string database, std::string table, std::string pwd, IndexManager* indexManager) {

    if (batch.empty()) {
        pqxx::result r = indexManager->getDocsToScore();
        for (pqxx::result::const_iterator row = r.begin(); row != r.end(); ++row) {
            batch.push_back(atoi((row)[0].c_str()));
        }
    }

    // since this can be a thread so we need a new connection for each call.

    // this statement calculates the idf

    std::vector<std::pair<std::string,float>> grams;

    for (std::vector<int>::iterator it_ = batch.begin(); it_ != batch.end(); it_++) {

        float multiplier = 1.0;

        for (std::vector<std::string>::const_iterator it = indexManager->unibitri.begin(); it != indexManager->unibitri.end(); it++) {

            pqxx::result rgt = indexManager->updateDocScore(*it, *it_);

            // Enitiy extraction. this should be deleted and moved to ML
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
        indexManager->processDocEntites(strarray, *it_);
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
    seg.parse(id, lang, rawdoc, doc_unigram_map, doc_bigram_map, doc_trigram_map, stopSuggest[lang]);
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

void IndexManager::updateStopSuggest(std::string lang, int batchsize) {
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
        pqxx::result r = txn.exec_prepared("update_stop_suggest", lang, gram, it->first.at(0), it->second/batchsize);
    }
    txn.commit();
}

void IndexManager::getMaxDocId(int &num) {
    pqxx::work txn(*C);
    pqxx::result r = txn.exec_prepared("max_doc_id");
    txn.commit();
    const pqxx::field c = r.back()[0];
    num = atoi(c.c_str());
}

void IndexManager::prepare_update_stop_suggest() {
    C->prepare("update_stop_suggest", "INSERT INTO stop_suggest (lang,gram,stop,idf) VALUES($1,$2,$3,$4) ON CONFLICT ON CONSTRAINT stop_suggest_lang_gram_key DO UPDATE SET idf = ((stop_suggest.idf + $4)/2) WHERE stop_suggest.lang = $1 AND stop_suggest.gram = $2");
}

void IndexManager::prepare_docscore_batch() {
    C->prepare("docscore_batch", "SELECT id, lang FROM \"" + table + "\" WHERE segmented_grams IS NOT NULL AND entities IS NULL LIMIT 10");
}

void IndexManager::prepare_max_doc_id() {
    C->prepare("max_doc_id", "SELECT MAX(id) FROM \"" + table +"\"");
}

void IndexManager::prepare_docs_batch() {
    std::string statement = "SELECT id,url,concat(" + columns + ") as document,lang FROM \"" + table + "\" WHERE (feed_date > index_date OR index_date IS null) LIMIT $1";
    C->prepare("process_docs_batch", statement);
}

pqxx::result IndexManager::getDocsToScore() {
    pqxx::work txn(*C_);
    pqxx::result r = txn.exec_prepared("docs_to_score");
    txn.commit();
    return r;
}

void IndexManager::processDocEntites(std::string str, int doc_id) {
    pqxx::work txn(*C_);
    pqxx::result r = txn.exec_prepared("process_doc_entities", str, doc_id);
    txn.commit();
}

/*
 * Update docscore
 */
pqxx::result IndexManager::updateDocScore(std::string gram, int doc_id) {

    pqxx::work txn(*C_);
    // update the docuscore using the idf / term frequencies
    pqxx::result rds = txn.exec_prepared("process_" + gram + "_docscore", doc_id);

    // basic entity extraction function.
    // this should be deleted and moved to ML
    pqxx::result r = txn.exec_prepared("process_"+ gram +"_batch", doc_id);
    txn.commit();
    return r;
}

pqxx::result IndexManager::getDocsToPurge() {
    pqxx::work txn(*C_);
    pqxx::result r = txn.exec_prepared("get_updated_docs");
    txn.commit();
    return r;
}

void IndexManager::purgeDocs(int id, std::string doc) {
    pqxx::work txn(*C_);
    pqxx::result r = txn.exec_prepared("update_purged_docs", id, doc);
    txn.commit();
}

void IndexManager::getNumDocs(std::map<std::string, int> &count) {
    pqxx::work txn(*C_);
    pqxx::result r = txn.exec_prepared("doc_count");
    txn.commit();
    int total = 0;
    for (pqxx::result::const_iterator row = r.begin(); row != r.end(); ++row) {
        const pqxx::field l = (row)[0];
        const pqxx::field c = (row)[1];
        count[l.c_str()] = atoi(c.c_str());
        total += atoi(c.c_str());
    }
}

void IndexManager::prepare_process_batch(std::string gram) {
    std::string statement = "SELECT d.id, key, (CHAR_LENGTH(value) - CHAR_LENGTH(REPLACE(value, ',', ''))), " + gram + ".idf AS i FROM \"" + table + "\" d, jsonb_each_text(d.segmented_grams->'"+ gram +"') docterms INNER JOIN "+ gram +" ON docterms.key = "+ gram +".gram WHERE d.id = $1 ORDER BY i DESC LIMIT 30";
    std::cout << " DEBUG " << "process_" << gram << "_batch" << std::endl; 
    C_->prepare("process_" + gram + "_batch", statement);
}

void IndexManager::prepare_update_docscore(std::string gram) {
    std::string statement = "WITH v AS (WITH d AS (SELECT docterms.key, max(array_length(regexp_split_to_array(docterms.value, ','), 1)), lang FROM \"" + table + "\" d, jsonb_each_text(d.segmented_grams->'" + gram + "') docterms WHERE d.id = $1 GROUP BY docterms.key, lang) SELECT DISTINCT (SUM(d.max) OVER()) AS freq, (SUM(d.max * t.idf) OVER()) AS score FROM d INNER JOIN " + gram + " AS t ON d.key = t.gram WHERE d.lang = t.lang GROUP BY d.max, t.idf) UPDATE \"" + table + "\" SET docscore = (SELECT score/freq FROM v) WHERE id = $1";
    C_->prepare("process_" + gram + "_docscore", statement);
}

void IndexManager::prepare_docs_to_score() {
    C_->prepare("docs_to_score", "SELECT id FROM \"" + table + "\" WHERE index_date IS NOT null AND docscore IS null");
}

void IndexManager::prepare_process_doc_entites() {
    std::string statement = "UPDATE \"" + table + "\" SET entities = $1 WHERE id = $2";
    C_->prepare("process_doc_entities", statement);
}

void IndexManager::prepare_get_docs() {
    std::string statement = "SELECT id, index_date FROM \"" + table + "\" WHERE update = true";
    C_->prepare("get_updated_docs", statement);
}

void IndexManager::prepare_doc_count() {
    C_->prepare("doc_count", "SELECT lang, COUNT(lang) AS count FROM \"" + table + "\" GROUP BY lang");
}

void IndexManager::prepare_purge_docs() {
    std::string statement = "UPDATE \"" + table + "\" SET update = false WHERE id = $1 AND index_date = $2";
    C_->prepare("update_purged_docs", statement);
}
