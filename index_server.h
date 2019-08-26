#ifndef _INDEX_SERVER_H_
#define _INDEX_SERVER_H_

#include <string>
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <pqxx/pqxx>
#include <future>
#include "query_builder.h"

class IndexServer {
	private:
		pqxx::connection* C;
		pqxx::work* txn;

	public:
		IndexServer();
		~IndexServer();
		void init();
		void execute(std::string lang, std::string query, std::promise<std::string> promiseObj);

};

#endif

