#include "index_server.h"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include "base64.h"
#include <algorithm>
#include "query.h"
#include <ctime>
#include "texttools.h"
#include "dirent.h"


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
	//std::string ngrams[] = {"uni","bi","tri"};
	//std::string langs[] = {"en","ja","zh"};
	std::string ngrams[] = {"uni"};
	std::string langs[] = {"en"};
	for (const string &ng : ngrams) {
		for (const string &l : langs) {
			loadIndex(ng, l);
		}
	}
}

void IndexServer::loadIndex(std::string ng, std::string lang) {

	time_t beforeload = time(0);
	std::cout << "loading index... (this might take a while)." << std::endl;


	std::vector<std::string> index_files;
	std::string path = "index/";

	struct dirent *entry;
	DIR *dp;

	dp = opendir(path.c_str());
	if (dp == NULL) {
		perror("opendir");
		std::cout << "shard_manager.cc : Error , unable to load last shard" << std::endl;;
		exit;
	}
	std::string ext = ".shard";
	while (entry = readdir(dp)) {
		std::string e_(entry->d_name);
		if ((e_.find(ext) != std::string::npos)) {
			if (e_.substr(e_.length()-6).compare(".shard") == 0) {
				index_files.push_back(entry->d_name);
			}
		}
	}
	closedir(dp);

	std::sort(index_files.begin(),index_files.end());
	if (index_files.empty()) {
		std::cout << "no index files create new shard" << std::endl;
		return;
	} else {
		std::cout << "unigramurls_map.size() " << unigramurls_map.size() << std::endl;
		for (std::vector<std::string>::iterator it = index_files.begin() ; it != index_files.end(); ++it) {
			int shard_id = stoi((*it).substr((*it).find('_')+1,(*it).find('.')));
			Shard shard(Shard::Type::UNIGRAM, shard_id);
			std::cout << *it << std::endl;
			shard.addToIndex(unigramurls_map);
			std::cout << "unigramurls_map.size() " << unigramurls_map.size() << std::endl;
		}
	}

	/*
	pqxx::work txn(*C);
	C->prepare("load_"+ng+"gram_"+lang+"_urls_batch", "SELECT "+ng+"grams_"+lang+".gram, array_agg(url_id)::int[] FROM (SELECT gram_id, url_id, weight FROM docunigrams_en ORDER BY score) AS dng INNER JOIN "+ng+"grams_"+lang+" ON ("+ng+"grams_"+lang+".id = dng.gram_id) GROUP BY "+ng+"grams_"+lang+".gram");

	pqxx::result r = txn.prepared("load_"+ng+"gram_"+lang+"_urls_batch").exec();
	std::cout << "index_server.cc " << ng << "gram database query complete processing.." << std::endl;

	// int t = 0;
	for (pqxx::result::const_iterator row = r.begin(); row != r.end(); ++row) {
		const pqxx::field gram = (row)[0];
		const pqxx::field urls = (row)[1];
		const char* urls_c = urls.c_str();
		if (gram.is_null()) {
			std::cout << "index_server.cc skip : url is null" << std::endl;;
			continue;
		} else {
			std::vector<int> gramurls; // mximum no of grams per url
			csvToIntVector(urls.as<std::string>(), gramurls);
			
			int k=0; // char array position tracker
			char j[10]; // url
			-- delete below --
			for (int i=1; i<strlen(urls_c)-1; i++) {
				if (i > 100000) {
					break;
				}
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
			-- delete above --
			if (ng == "uni") {
				unigramurls_map.insert(std::pair<std::string, std::vector<int>>(gram.as<std::string>(),gramurls));
			} else if (ng == "bi") {
				bigramurls_map.insert(std::pair<std::string, std::vector<int>>(gram.as<std::string>(),gramurls));
			} else if (ng == "tri") {
				trigramurls_map.insert(std::pair<std::string, std::vector<int>>(gram.as<std::string>(),gramurls));
			} else {
				continue;
			}
		}
		if (urls.is_null()) {
			std::cout << "index_server.cc skip : feed is null" << std::endl;;
			continue;
		}
	}
	txn.commit();
	*/
	time_t afterload = time(0);
	double seconds = difftime(afterload, beforeload);
	std::cout << "index_server.cc finished loading " << ng << "gram " << lang << " index in " << seconds << " seconds." << std::endl;
	/*
	for (std::unordered_map<std::string, std::vector<int>>::iterator it = ngramurls_map.begin() ; it != ngramurls_map.end(); ++it) {
		std::cout << "index_server.cc :"  << it->first << ":" << std::endl;
	}
	*/
}

void IndexServer::execute(std::string lang, std::string parsed_query, std::promise<std::string> promiseObj) {
	std::thread th(search, lang, parsed_query, std::move(promiseObj), this);
	th.join();
}

/*
 * In a massively scaled out version these would all be separate services with their own network stack
 * The query would run to various servelets getting rewritten and then results retrieved and scored.
 */
void IndexServer::search(std::string lang, std::string parsed_query, std::promise<std::string> promiseObj, IndexServer *indexServer) {
	QueryBuilder queryBuilder;
	Query::Node query;
	//Result::Item item;
	queryBuilder.build(lang, parsed_query, query);
	// TODO update query with other stuff.
	// queueRewrite->spellingServlet.correctSpelling();
	// queueRewrite->synsServlet.addSynonyms();
	// queueRewrite->coneptServlet.addConcepts();
	// queueRewrite->otherServlet.addOther();
	// indexServer->retrieveResults();
	// indexServer->resolveQuery(query, indexServer);
	std::vector<Query::Term> candidates;
	indexServer->addQueryCandidates(query, indexServer, candidates);
	std::sort(candidates.begin(), candidates.end(),
		[](const Query::Term& l, const Query::Term& r) {
		return l.weight > r.weight;
	});
	query.candidates = candidates;
	//cScorer.score(&result);
	std::string result = query.serialize();
	promiseObj.set_value(result);
}


/*
 * Retrieval function populate the query
 */
void IndexServer::addQueryCandidates(Query::Node &query, IndexServer *indexServer, std::vector<Query::Term> &candidates) {

	std::cout << "index_server.cc : add query candidates" << std::endl;
	if (query.leafNodes.size()==0) {
		std::string converted;
		query.term.term.toUTF8String(converted);
		std::cout << "index_server.cc - looking for " << converted << std::endl;

		phmap::parallel_flat_hash_map<std::string, std::vector<Shard::Term>>::const_iterator urls = indexServer->unigramurls_map.find(converted);
		if (urls != indexServer->unigramurls_map.end()) {
			// std::cout << "index_server.cc Found " << converted << std::endl;
			// std::cout << "index_server.cc Debug " << urls->first << std::endl;

			for (std::vector<Shard::Term>::const_iterator it = urls->second.begin(); it != urls->second.end(); ++it) {
				Query::Term term;
				term.tf=it->tf;
				term.weight=it->weight;
				term.debug_url_id=it->url_id;

				pqxx::work txn(*C);
				C->prepare("get_url","SELECT url FROM docs_en WHERE id = $1");
				pqxx::result r = txn.prepared("get_url")(it->url_id).exec();
				txn.commit();
				const pqxx::field c = r.back()[0];
				// std::cout << "index_server.cc : debug url id - " << it->url_id << std::endl;
				// std::cout << "index_server.cc : debug c - " << pqxx::to_string(c) << std::endl;
				term.debug_url=pqxx::to_string(c);

				candidates.push_back(term);
				// query.candidates.push_back(term);

				if (it == urls->second.begin()+MAX_CANDIDATES_COUNT) {
					break;
				}
			}
		}
	} else {
		std::vector<Query::Term> node_candidates;
		for (std::vector<Query::Node>::iterator it = query.leafNodes.begin() ; it != query.leafNodes.end(); ++it) {
			std::vector<Query::Term> candidates_;
			addQueryCandidates(*it, indexServer, candidates_);
			if (node_candidates.empty()) {
				node_candidates=candidates_;
			} else {
				std::vector<Query::Term> new_candidates;
				for (std::vector<Query::Term>::const_iterator tit = candidates_.begin(); tit != candidates_.end(); ++tit) {
					// introduce AND , OR logic here.
					auto ait = find_if(node_candidates.begin(), node_candidates.end(), [tit](const Query::Term t) {
							return t.debug_url_id == tit->debug_url_id;
					});
					if (ait != node_candidates.end()) {
						ait->weight=ait->weight + tit->weight;
						new_candidates.push_back(*ait);
					}
				}
				if (new_candidates.empty()) {
					node_candidates.clear();
				} else {
					node_candidates = new_candidates;
				}
			}
		}
		candidates = node_candidates;
	}
}


/*
 * Retrieval function populate the query
 */
/*
void IndexServer::addQueryCandidates(Query::Node &query, IndexServer *indexServer) {

	std::cout << "index_server.cc : add query candidates" << std::endl;
	if (!query.term.term.isEmpty()) {
		std::string converted;
		query.term.term.toUTF8String(converted);
		std::cout << "index_server.cc - looking for " << converted << std::endl;

		phmap::parallel_flat_hash_map<std::string, std::vector<Shard::Term>>::const_iterator urls = indexServer->unigramurls_map.find(converted);
		if (urls != indexServer->unigramurls_map.end()) {
			// std::cout << "index_server.cc Found " << converted << std::endl;
			// std::cout << "index_server.cc Debug " << urls->first << std::endl;

			for (std::vector<Shard::Term>::const_iterator it = urls->second.begin(); it != urls->second.end(); ++it) {
				Query::Term term;
				term.tf=it->tf;
				term.weight=it->weight;
				term.debug_url_id=it->url_id;

				pqxx::work txn(*C);
				C->prepare("get_url","SELECT url FROM docs_en WHERE id = $1");
				pqxx::result r = txn.prepared("get_url")(it->url_id).exec();
				txn.commit();
				const pqxx::field c = r.back()[0];

				// std::cout << "index_server.cc : debug url id - " << it->url_id << std::endl;
				// std::cout << "index_server.cc : debug c - " << pqxx::to_string(c) << std::endl;

				term.debug_url=pqxx::to_string(c);
				query.candidates.push_back(term);

				if (it == urls->second.begin()+MAX_CANDIDATES_COUNT) {
					break;
				}
			}
		}
	} else {
		std::cout << "index_server.cc empty query node term seen." << std::endl;
	}
	for (std::vector<Query::Node>::iterator it = query.leafNodes.begin() ; it != query.leafNodes.end(); ++it) {
		addQueryCandidates(*it, indexServer);
	}
}
*/

