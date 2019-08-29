#include "index_server.h"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include "base64.h"
#include <algorithm>
#include "query.h"


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
	std::cout << "drop this" << std::endl;
	// returns ngram id and urls ids
	// C->prepare("load_gramurls_batch", "SELECT gram_id, array_agg(url_id) FROM (SELECT gram_id, url_id, incidence FROM docngrams WHERE gram_id IN ($1,$2) ORDER BY incidence DESC) AS sub GROUP BY sub.gram_id");
	// returns string ngram by id and urls ids
	C->prepare("load_gramurls_batch", "SELECT ngrams.gram, array_agg(url_id)::int[] FROM (SELECT gram_id, url_id, incidence FROM docngrams WHERE gram_id BETWEEN $1 AND $2 ORDER BY incidence DESC) AS _ng INNER JOIN ngrams ON (ngrams.id = _ng.gram_id) GROUP BY ngrams.gram");
	pqxx::work txn(*C);
	pqxx::result r = txn.prepared("load_gramurls_batch")("1")("1000000").exec();
	int t = 0;
	for (pqxx::result::const_iterator row = r.begin(); row != r.end(); ++row) {
		const pqxx::field gram = (row)[0];
		const pqxx::field urls = (row)[1];
		// std::cout << " - - - - - " << std::endl;
		std::cout << "gram : " << t << " " << gram.c_str() << std::endl;
		t++;
		// std::cout << "urls : " << urls.c_str() << std::endl;
		const char* urls_c = urls.c_str();
		if (gram.is_null()) {
			std::cout << "skip : url is null" << std::endl;;
			continue;
		} else {
			int k=0; // char array position tracker
			char j[10]; // url
			std::vector<int> gramurls; // mximum no of grams per url
			for (int i=1; i<strlen(urls_c)-1; i++) {
				if (i > 100000) {
					break;
				}
				// std::cout << "i " << (urls_c)[i] << std::endl;
				if ((urls_c)[i]==',') {
					char u[k];
					strncpy(u, j, k);
					gramurls.push_back(atoi(u));
					k=0;
					j[0] = '\0';
					continue;
				} else {
					j[k]=(urls_c)[i];
					k++;
				}
			}
			ngramurls_map[gram.as<std::string>()]=gramurls;
		}
		if (urls.is_null()) {
			std::cout << "skip : feed is null" << std::endl;;
			continue;
		}
	}
	/*
	(/

	// buildIndex();
	// spp.init();

	// this is a redis connection (were replacing this with postgres for the index)
	//	client.connect();
	// postgres connection
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
	Query::Node query;
	queryBuilder.build(lang, parsed_query, query);
	result = query.serialize();
	promiseObj.set_value(result);
}

