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
#include "shard.h"
#include "parallel_hashmap/phmap.h"

class IndexServer {

	public:
		IndexServer();
		~IndexServer();
		void init();
		void addQueryCandidates(Query::Node &query, IndexServer *indexServer, std::vector<Query::Term> &candidates);
		void execute(std::string lang, std::string query, std::promise<std::string> promiseObj);
		static void search(std::string lang, std::string parsed_query, std::promise<std::string> promiseObj, IndexServer *indexServer);

	private:
		pqxx::connection* C;
		pqxx::work* txn;
		phmap::parallel_flat_hash_map<std::string, std::vector<Shard::Term>> unigramurls_map;
		std::unordered_map<std::string, std::map<int, Shard::Term>> bigramurls_map;
		std::unordered_map<std::string, std::map<int, Shard::Term>> trigramurls_map;
		void loadIndex(std::string gram, std::string lang);
		const int MAX_CANDIDATES_COUNT = 1000;

};

#endif

