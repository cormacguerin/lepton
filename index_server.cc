#include "index_server.h"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include "base64.h"
#include <algorithm>
#include "query.h"
#include <ctime>
#include "texttools.h"


using namespace std;
using namespace pqxx;


IndexServer::IndexServer()
{
	init();
}

IndexServer::~IndexServer()
{
}

void IndexServer::init() {
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
	time_t beforeload = time(0);
	std::cout << "loading index... (this might take a while)." << std::endl;
	// returns ngram id and urls ids
	// C->prepare("load_gramurls_batch", "SELECT unigrams_en.gram, array_agg(url_id)::int[] FROM (SELECT gram_id, url_id, incidence FROM docunigrams_en WHERE gram_id BETWEEN $1 AND $2 ORDER BY incidence DESC) AS _ng INNER JOIN unigrams_en ON (unigrams_en.id = _ng.gram_id) GROUP BY unigrams_en.gram");
	C->prepare("load_gramurls_batch", "SELECT unigrams_en.gram, array_agg(url_id)::int[] FROM (SELECT gram_id, url_id, incidence FROM docunigrams_en ORDER BY incidence DESC) AS _ng INNER JOIN unigrams_en ON (unigrams_en.id = _ng.gram_id) GROUP BY unigrams_en.gram");

	pqxx::work txn(*C);
	//pqxx::result r = txn.prepared("load_gramurls_batch")("1")("5961415").exec();
	pqxx::result r = txn.prepared("load_gramurls_batch").exec();

	time_t afterload = time(0);
	double seconds = difftime(beforeload,afterload);
	std::cout << "finished loading index in " << seconds << " seconds." << std::endl;
	// int t = 0;
	for (pqxx::result::const_iterator row = r.begin(); row != r.end(); ++row) {
		const pqxx::field gram = (row)[0];
		const pqxx::field urls = (row)[1];
		// std::cout << "gram : " << t << " " << gram.c_str() << std::endl;
		// t++;
		const char* urls_c = urls.c_str();
		if (gram.is_null()) {
			std::cout << "skip : url is null" << std::endl;;
			continue;
		} else {
			std::vector<int> gramurls; // mximum no of grams per url
			csvToIntVector(urls.as<std::string>(), gramurls);
			/*
			int k=0; // char array position tracker
			char j[10]; // url
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
			*/
			unigramurls_map.insert(std::pair<std::string, std::vector<int>>(gram.as<std::string>(),gramurls));
		}
		if (urls.is_null()) {
			std::cout << "skip : feed is null" << std::endl;;
			continue;
		}
	}
	std::cout << "Index Loaded." << std::endl;;
	/*
	for (std::unordered_map<std::string, std::vector<int>>::iterator it = ngramurls_map.begin() ; it != ngramurls_map.end(); ++it) {
		std::cout << ":"  << it->first << ":" << std::endl;
	}
	*/
}

void IndexServer::execute(std::string lang, std::string parsed_query, std::promise<std::string> promiseObj) {
	std::thread th(search, lang, parsed_query, std::move(promiseObj), this);
	th.join();
}

void IndexServer::search(std::string lang, std::string parsed_query, std::promise<std::string> promiseObj, IndexServer *indexServer) {
	QueryBuilder queryBuilder;
	Query::Node query;
	queryBuilder.build(lang, parsed_query, query);
	indexServer->addQueryCandidates(query, indexServer);
	std::string result = query.serialize();
	promiseObj.set_value(result);
}

/*
 * Function to populate the query with the best candidate urls.
 * TODO: this just returns urls in order of the incidence of the term.
 * We need a better way to return the 'best' doc matches, to do that we should..
 * - sort by idf rather than incidence
 * - post filter based on pagerank maybe..
 */
void IndexServer::addQueryCandidates(Query::Node &query, IndexServer *indexServer) {
	std::cout << "add query canditates" << std::endl;
	if (!query.term.term.isEmpty()) {
		std::string converted;
		query.term.term.toUTF8String(converted);
		std::cout << "- looking for " << converted << std::endl;
		std::unordered_map<std::string,std::vector<int>>::const_iterator urls = indexServer->unigramurls_map.find(converted);
		if (urls != indexServer->unigramurls_map.end()) {
			std::cout << "Found " << converted << std::endl;
			query.candidates=urls->second;
		}
	} else {
		std::cout << "empty term" << std::endl;
	}
	for (std::vector<Query::Node>::iterator it = query.leafNodes.begin() ; it != query.leafNodes.end(); ++it) {
		addQueryCandidates(*it, indexServer);
	}
}

