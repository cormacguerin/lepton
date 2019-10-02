#ifndef _INDEX_MANAGER_H_
#define _INDEX_MANAGER_H_

#include <string>
//#include "sentence_piece_processor.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <pqxx/pqxx>
#include "segmenter.h"
#include "frag_manager.h"

class IndexManager {
	private:
		// SentencePieceProcessor spp;
		Segmenter seg;
		FragManager fragManager;
		pqxx::connection* C;
		char SPS[4];
		std::string SPC;

	public:
		IndexManager();
		~IndexManager();
		void quit( int code );
		void init();
		void prepare_export_vocab(pqxx::connection_base &c, std::string lang);
		void prepare_doc_count(pqxx::connection_base &c, std::string lang);
		void prepare_unigram_count(pqxx::connection_base &c, std::string lang);
		void prepare_bigram_count(pqxx::connection_base &c, std::string lang);
		void prepare_trigram_count(pqxx::connection_base &c, std::string lang);
		void prepare_update_unigram_idf(pqxx::connection_base &c, std::string lang);
		void prepare_update_bigram_idf(pqxx::connection_base &c, std::string lang);
		void prepare_update_trigram_idf(pqxx::connection_base &c, std::string lang);
		void prepare_unigram_document_frequency(pqxx::connection_base &c, std::string lang);
		void prepare_bigram_document_frequency(pqxx::connection_base &c, std::string lang);
		void prepare_trigram_document_frequency(pqxx::connection_base &c, std::string lang);
		void prepare_max_doc_id(pqxx::connection_base &c, std::string lang);
		void prepare_max_unigram_id(pqxx::connection_base &c, std::string lang);
		void prepare_max_bigram_id(pqxx::connection_base &c, std::string lang);
		void prepare_max_trigram_id(pqxx::connection_base &c, std::string lang);
		void getMaxDocId(int &num, std::string lang);
		void getNumDocs(int &count, std::string lang);
		void getNumNgrams(int &count, std::string gram, std::string lang);
		void getMaxNgramId(int &numm, std::string gram, std::string lang);
		void updateNgramIdf(std::map<int, double> idfbatch, std::string gram, std::string lang);
		void updateIdf(std::string lang);
		void processFeeds(std::string lang);
		void exportVocab(std::string lang);
		bool isSPS(char firstchar);
		void indexDocument(std::string id, std::string key, std::string doc, std::string lang);

};

#endif

