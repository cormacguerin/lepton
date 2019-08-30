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

//class IndexServer : public std::enable_shared_from_this<IndexServer> {
class IndexServer {

	public:
		IndexServer();
		~IndexServer();
		void init();
		void addQueryCandidates(Query::Node &query, IndexServer *indexServer);
		void execute(std::string lang, std::string query, std::promise<std::string> promiseObj);
		static void search(std::string lang, std::string parsed_query, std::promise<std::string> promiseObj, IndexServer *indexServer);

	private:
		pqxx::connection* C;
		pqxx::work* txn;
		std::unordered_map<std::string, std::vector<int>> ngramurls_map;

};

#endif

