#ifndef _INDEX_MANAGER_H_
#define _INDEX_MANAGER_H_

#include <string>
//#include "sentence_piece_processor.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <pqxx/pqxx>
#include "segmenter.h"
#include <vector>
#include "frag_manager.h"

class IndexManager {
	private:
		// SentencePieceProcessor spp;
		Segmenter seg;
		pqxx::connection* C;
		char SPS[4];
		std::string SPC;
		FragManager unigramFragManager;
		FragManager bigramFragManager;
		FragManager trigramFragManager;
    std::string database;
    std::string table;

	public:
		IndexManager(Frag::Type u, Frag::Type b, Frag::Type t, std::string database, std::string table);
		~IndexManager();
		void quit( int code );
		void init(std::string database);
		void prepare_export_vocab(pqxx::connection_base &c, std::string lang);
		void prepare_doc_count(pqxx::connection_base &c, std::string lang);
		void prepare_unigram_count(pqxx::connection_base &c, std::string lang);
		void prepare_bigram_count(pqxx::connection_base &c, std::string lang);
		void prepare_trigram_count(pqxx::connection_base &c, std::string lang);
		void prepare_unigram_document_frequency(pqxx::connection_base &c, std::string lang);
		void prepare_bigram_document_frequency(pqxx::connection_base &c, std::string lang);
		void prepare_trigram_document_frequency(pqxx::connection_base &c, std::string lang);
		void prepare_max_doc_id(pqxx::connection_base &c, std::string table);
		void prepare_max_unigram_id(pqxx::connection_base &c, std::string lang);
		void prepare_max_bigram_id(pqxx::connection_base &c, std::string lang);
		void prepare_max_trigram_id(pqxx::connection_base &c, std::string lang);
		void prepare_docscore_batch(pqxx::connection_base &c, std::string table);
		void getMaxDocId(int &num, std::string table);
		void getNumDocs(int &count, std::string table);
		void getNumNgrams(int &count, std::string gram, std::string lang);
		void getMaxNgramId(int &numm, std::string gram, std::string lang);
		void updateIdf(std::string lang);
		void processFeeds(std::string table);
		void processDocInfo(std::string lang);
		void exportVocab(std::string lang);
		bool isSPS(char firstchar);
		bool hasDigit(const std::string& s);
		void indexDocument(std::string id, std::string display_field, std::string doc, std::string lang);
		std::vector<int> GetDocscoreBatch(std::string table);

};

#endif

