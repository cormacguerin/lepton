#include "index_server.h"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include "base64.h"
#include <algorithm>


using namespace std;
using namespace pqxx;


IndexServer::IndexServer()
{
	try {
		C = new pqxx::connection("dbname = index user = postgres password = kPwFWfYAsyRGZ6IomXLCypWqbmyAbK+gnKIW437QLjw= hostaddr = 127.0.0.1 port = 5432");
		if (C->is_open()) {
			cout << "Opened database successfully: " << C->dbname() << endl;
		} else {
			cout << "Can't open database" << endl;
		}
	} catch (const std::exception &e) {
		cerr << e.what() << std::endl;
	}
	init();
}

IndexServer::~IndexServer()
{
}

void IndexServer::init() {
	/*
	std::cout << "drop this" << std::endl;
	C->prepare("load_gramurls_batch", "SELECT gram_id, array_agg(url_id) FROM (SELECT gram_id, url_id, incidence FROM docngrams WHERE gram_id IN ($1,$2) ORDER BY incidence DESC) AS sub GROUP BY sub.gram_id");
	pqxx::work txn(*C);
	pqxx::result r = txn.prepared("load_gramurls_batch")("1")("20").exec();
	for (pqxx::result::const_iterator row = r.begin(); row != r.end(); ++row) {
		const pqxx::field urls = (row)[0];
		const pqxx::field gram = (row)[1];
		std::cout << " - - - - - " << std::endl;
		std::cout << "url : " << gram.c_str() << std::endl;
		std::cout << "url : " << urls.c_str() << std::endl;
		if (gram.is_null()) {
			std::cout << "skip : url is null" << std::endl;;
			continue;
		}
		if (urls.is_null()) {
			std::cout << "skip : feed is null" << std::endl;;
			continue;
		}
	}
	(/

	// buildIndex();
	// spp.init();

	// this is a redis connection (were replacing this with postgres for the index)
	//	client.connect();
	// postgres connection
	/*
	try {
		C = new pqxx::connection("dbname = index user = postgres password = kPwFWfYAsyRGZ6IomXLCypWqbmyAbK+gnKIW437QLjw= hostaddr = 127.0.0.1 port = 5432");
    	if (C->is_open()) {
    	   cout << "Opened database successfully: " << C->dbname() << endl;
    	} else {
    	   cout << "Can't open database" << endl;
    	}
	} catch (const std::exception &e) {
		cerr << e.what() << std::endl;
	}
	*/
}

void IndexServer::execute(std::string lang, std::string parsed_query, std::promise<std::string> promiseObj) {
	std::string result;
	QueryBuilder queryBuilder;
	queryBuilder.parse(lang, parsed_query, result);
	promiseObj.set_value(result);
}

