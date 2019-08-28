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

class IndexServer {
	private:
		pqxx::connection* C;
		pqxx::work* txn;
		std::unordered_map<std::string, std::vector<int>> ngramurls_map;

	public:
		IndexServer();
		~IndexServer();
		void init();
		static void execute(std::string lang, std::string query, std::promise<std::string> promiseObj);

};

#endif

