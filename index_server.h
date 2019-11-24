#ifndef _INDEX_SERVER_H_
#define _INDEX_SERVER_H_

#include <string>
#include <pqxx/pqxx>
#include <unordered_map>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <future>
#include "query_builder.h"
#include "frag.h"
#include "result.h"
#include "parallel_hashmap/phmap.h"

class IndexServer {

	public:
		IndexServer();
		~IndexServer();
		void init();
		void addQueryCandidates(Query::Node &query, IndexServer *indexServer, std::vector<Frag::Item> &candidates);
		void execute(std::string lang, std::string query, std::promise<std::string> promiseObj);
		static void search(std::string lang, std::string parsed_query, std::promise<std::string> promiseObj, IndexServer *indexServer, QueryBuilder queryParser);

	private:
		int q;
		int x;
		pqxx::connection* C;
		pqxx::work* txn;
		phmap::parallel_flat_hash_map<std::string, std::vector<Frag::Item>> unigramurls_map;
		phmap::parallel_flat_hash_map<std::string, std::vector<Frag::Item>> bigramurls_map;
		phmap::parallel_flat_hash_map<std::string, std::vector<Frag::Item>> trigramurls_map;
		std::vector<std::string> getDocInfo(int url_id);
		std::map<std::string,std::vector<int>> getTermPositions(int url_id, std::vector<std::string> terms);
		Result getResult(std::vector<std::string> terms, std::vector<Frag::Item> candidates);
		void getResultInfo(Result& result);
		pqxx::prepare::invocation& prep_dynamic(std::vector<std::string> data, pqxx::prepare::invocation& inv);
		void loadIndex(std::string gram, std::string lang);
		QueryBuilder queryBuilder;
		const int MAX_CANDIDATES_COUNT = 1000;
		int getTime();

};

#endif

