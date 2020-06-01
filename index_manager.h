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
		// SentencePieceProcessor spp;
		Segmenter seg;
		pqxx::connection* C;
		char SPS[4];
        std::vector<std::string> langs = {"en","ja","zh","ko","es","de","fr"};
		std::string SPC;
        // per language map per language frag manager (can be large so put on heap)
        // ie. <lang , fragmanager >
		std::map<std::string, FragManager*> unigramFragManager;
		std::map<std::string, FragManager*> bigramFragManager;
        std::map<std::string, FragManager*> trigramFragManager;
        std::map<std::string, Frag::Item> doc_unigram_map;
        std::map<std::string, Frag::Item> doc_bigram_map;
        std::map<std::string, Frag::Item> doc_trigram_map;


        // these two bools control the status of running threads
        // process_feeds - the actual process feeds run status.
        // merge_frags - the actual merge frags run status.
        bool process_feeds;
        bool merge_frags;

	public:
		IndexManager(Frag::Type u, Frag::Type b, Frag::Type t, std::string database, std::string table, std::string columns);
		~IndexManager();
        // do_run - the intended status set from outside the class
        bool do_run;

        std::string database;
        std::string table;
        std::string columns;
        std::string display_field;
		void quit( int code );
		void init();
		void prepare_export_vocab(pqxx::connection_base &c, std::string lang);
		void prepare_doc_count(pqxx::connection_base &c);
		void prepare_unigram_count(pqxx::connection_base &c, std::string lang);
		void prepare_bigram_count(pqxx::connection_base &c, std::string lang);
		void prepare_trigram_count(pqxx::connection_base &c, std::string lang);
		void prepare_unigram_document_frequency(pqxx::connection_base &c, std::string lang);
		void prepare_bigram_document_frequency(pqxx::connection_base &c, std::string lang);
		void prepare_trigram_document_frequency(pqxx::connection_base &c, std::string lang);
		void prepare_max_doc_id(pqxx::connection_base &c);
		void prepare_max_unigram_id(pqxx::connection_base &c, std::string lang);
		void prepare_max_bigram_id(pqxx::connection_base &c, std::string lang);
		void prepare_max_trigram_id(pqxx::connection_base &c, std::string lang);
		void prepare_docscore_batch(pqxx::connection_base &c);
		void getMaxDocId(int &num);
        void getNumDocs(std::map<std::string, int> &count);
		void getNumNgrams(int &count, std::string gram, std::string lang);
		void getMaxNgramId(int &numm, std::string gram, std::string lang);
		void updateIdf(std::string lang);
		void processFeeds();
        void spawnProcessFeeds();
		static void processDocInfo(std::vector<int> ids, std::string database, std::string table, std::string password);
		void exportVocab(std::string lang);
		bool isSPS(char firstchar);
		void indexDocument(std::string id, std::string doc, std::string lang);
        static void runFragMerge(IndexManager* indexManager);
		std::vector<int> GetDocscoreBatch();

};

#endif

