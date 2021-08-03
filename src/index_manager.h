
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


#ifndef _INDEX_MANAGER_H_
#define _INDEX_MANAGER_H_

#include <string>
//#include "sentence_piece_processor.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <pqxx/pqxx>
#include <atomic>
#include <vector>
#include "segmenter.h"
#include "frag_manager.h"

class IndexManager {
	private:
    bool merge_frags;
		// SentencePieceProcessor spp;
		Segmenter seg;
		pqxx::connection* C;
		pqxx::connection* C_;
    pqxx::work* txn_;
		char SPS[4];
    std::vector<std::string> langs{"en","ja","zh","ko","es","de","fr"};
    std::vector<std::string> unibitri{"trigrams","bigrams","unigrams"};
		std::string SPC;

    // per language map per language frag manager (can be large so put on heap)
    // ie. <lang , fragmanager >

		std::map<std::string, FragManager*> unigramFragManager;
		std::map<std::string, FragManager*> bigramFragManager;
    std::map<std::string, FragManager*> trigramFragManager;
    std::map<std::string, std::map<std::vector<std::string>,double>> stopSuggest;

		void prepare_export_vocab(std::string lang);
		void prepare_max_doc_id();
		void prepare_max_unigram_id(std::string lang);
		void prepare_max_bigram_id(std::string lang);
		void prepare_max_trigram_id(std::string lang);
    void prepare_process_batch(std::string gram);
    void prepare_process_doc_entites();
    void prepare_update_docscore(std::string gram);
		void prepare_docscore_batch();
		void prepare_doc_count();
    void prepare_get_docs();
    void prepare_purge_docs();
    void prepare_update_stop_suggest();
    void prepare_docs_batch();
    void prepare_docs_to_score();
    void purgeDocs(int id, std::string docs);
    void commitTxn();
    void initTxn();
    void processDocEntites(std::string str, int doc_id);
    pqxx::result updateDocScore(std::string gram, int doc_id);


	public:
		IndexManager(Frag::Type u, Frag::Type b, Frag::Type t, std::string database, std::string table, std::string columns);
		~IndexManager();

    // do_run - the intended status set from outside the class
    bool do_run;
		bool isSPS(char firstchar);

    std::string database;
    std::string table;
    std::string columns;
    std::string display_field;
		void quit( int code );
		void init();
		void getMaxDocId(int &num);
    void getNumDocs(std::map<std::string, int> &count);
		void updateIdf(std::string lang);
    void updateStopSuggest(std::string lang, int batchsize);
		void processFeeds();
    void spawnProcessFeeds();
		void indexDocument(std::string id, std::string doc, std::string lang);
		static void processDocInfo(std::vector<int> ids, std::string database, std::string table, std::string password, IndexManager* indexManager);
    static void runFragMerge(IndexManager* indexManager);
    pqxx::result getDocsToScore();
    pqxx::result getDocsToPurge();
};

#endif

