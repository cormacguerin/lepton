#include "index_server.h"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include "base64.h"
#include <algorithm>
#include <ctime>
#include "query.h"
#include "texttools.h"
#include "dirent.h"


using namespace std;
using namespace pqxx;


IndexServer::IndexServer()
{
	q = 0;
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
		std::cout << "frag_manager.cc : Error , unable to load last frag" << std::endl;;
		exit;
	}
	std::string ext = ".frag";
	while (entry = readdir(dp)) {
		std::string e_(entry->d_name);
		if ((e_.find(ext) != std::string::npos)) {
			if (e_.substr(e_.length()-5).compare(".frag") == 0) {
				index_files.push_back(entry->d_name);
			}
		}
	}
	closedir(dp);

	std::sort(index_files.begin(),index_files.end());
	if (index_files.empty()) {
		std::cout << "no index files create new frag" << std::endl;
		return;
	} else {
		std::cout << "unigramurls_map.size() " << unigramurls_map.size() << std::endl;
		for (std::vector<std::string>::iterator it = index_files.begin() ; it != index_files.end(); ++it) {
			int frag_id = stoi((*it).substr((*it).find('_')+1,(*it).find('.')));
			Frag frag(Frag::Type::UNIGRAM, frag_id);
			std::cout << *it << std::endl;
			frag.addToIndex(unigramurls_map);
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

	time_t beforeload = time(0) * 1000;

	std::thread th(search, lang, parsed_query, std::move(promiseObj), this, queryBuilder);
	th.join();

	time_t afterload = time(0) * 1000;
	double seconds = difftime(afterload, beforeload);
	std::cout << "index_server.cc search " << parsed_query << " executed in " << seconds << " seconds." << std::endl;
}

/*
 * In a massively scaled out version these would all be separate services with their own network stack
 * The query would run to various servelets getting rewritten and then results retrieved and scored.
 */
void IndexServer::search(std::string lang, std::string parsed_query, std::promise<std::string> promiseObj, IndexServer *indexServer, QueryBuilder queryBuilder) {
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
	std::vector<Frag::Item> candidates;
	indexServer->addQueryCandidates(query, indexServer, candidates);

	std::vector<std::string> terms = query.getTerms();

	Result result = indexServer->getResult(terms, candidates);
	result.query = query;
	
	// cScorer.score(&result);
	std::sort(result.items.begin(), result.items.end(),
		[](const Result::Item& l, const Result::Item& r) {
		return l.score > r.score;
	});
	if (result.items.size() > 50) {
		result.items.resize(50);
	}
	promiseObj.set_value(result.serialize());
}

Result IndexServer::getResult(std::vector<std::string> terms, std::vector<Frag::Item> candidates) {

	Result result;

	q++;
	int p = 0;
	std::string prepstr="(";
	pqxx::work txn(*C);
	for (std::vector<std::string>::const_iterator it = terms.begin(); it != terms.end(); ++it) {
		p++;
		prepstr += "$" + std::to_string(p);
		if (std::next(it) != terms.end()) {
			prepstr += ",";
		}
	}
	prepstr += ")";

	std::map<int,int> c_map;
	int x = 0;
	std::string prepstr_="(";
	for (std::vector<Frag::Item>::const_iterator tit = candidates.begin(); tit != candidates.end(); ++tit) {
		Result::Item item;
		item.tf = tit->tf;
		item.weight = tit->weight;
		item.url_id = tit->url_id;
		result.items.push_back(item);
		prepstr_ += std::to_string(tit->url_id);
		if (std::next(tit) != candidates.end()) {
			prepstr_ += ",";
		}
		c_map[item.url_id]=x;
		x++;
	}
	prepstr_ += ")";

	C->prepare("get_docinfo"+q,"SELECT id, url, tdscore, docscore, key, value FROM docs_en d, jsonb_each_text(d.segmented_grams->'unigrams') docterms WHERE d.id IN " + prepstr_ + " AND docterms.key IN " + prepstr);

	std::map<std::string,std::vector<int>> term_positions;
	std::vector<pqxx::result> pqxx_results;

	pqxx::prepare::invocation w_invocation = txn.prepared("get_docinfo"+q);
	prep_dynamic(terms, w_invocation);
	pqxx::result r = w_invocation.exec();

	int k = 0;
	int m = 0;
	for (pqxx::result::const_iterator row = r.begin(); row != r.end(); ++row) {
		const pqxx::field i = (row)[0];
		const pqxx::field u = (row)[1];
		const pqxx::field q = (row)[2];
		const pqxx::field s = (row)[3];
		const pqxx::field term = (row)[4];
		const pqxx::field position = (row)[5];

		int id = c_map.at(atoi(i.c_str()));


		result.items.at(id).url = pqxx::to_string(u);
		result.items.at(id).tdscore = atof(q.c_str());
		result.items.at(id).docscore = atof(s.c_str());
		result.items.at(id).score = atof(s.c_str()) * result.items.at(id).weight;

		std::string term_(term.c_str());

		std::vector<int> positions;
		stringstream sterm(position.c_str());

		std::string p;
		while (getline(sterm, p, ',')) {
			positions.push_back(atoi(p.c_str()));
		}
		result.items.at(id).terms[term_] = positions;
	}
	txn.commit();

	// if this is the next url, the process the previous one testing for
	// adjecent terms in multi terms queries and prioritize if necessary
	for (std::vector<Result::Item>::iterator it = result.items.begin(); it != result.items.end(); ++it) {
	//	std::cout << " - - - - - " << std::endl;
		if (it->terms.size() > 1) {
			for (std::vector<std::string>::const_iterator s = terms.begin(); s != terms.end(); ++s) {
				if (std::next(s) == terms.end()) {
					break;
				}
				std::map<std::string,std::vector<int>>::iterator xit = it->terms.find(*s);
				std::map<std::string,std::vector<int>>::iterator yit = it->terms.find(*(std::next(s)));
				if (xit!=it->terms.end() && yit!=it->terms.end()) {
					int w=1;
					for (std::vector<int>::iterator zit = it->terms.at(*s).begin(); zit != it->terms.at(*s).end(); zit++) {
	//					std::cout << *s << " " << *zit << std::endl;
						w += std::count(it->terms.at(*(std::next(s))).begin(), it->terms.at(*(std::next(s))).end(), (*zit)+1);
					}
					it->score += log(w);
	//				for (std::vector<int>::iterator rit = it->terms.at(*std::next(s)).begin(); rit != it->terms.at(*std::next(s)).end(); rit++) {
	//					std::cout << (*std::next(s)) << " " << *rit << std::endl;
	//				}
				}
			}
		}
	}

	return result;
}


pqxx::prepare::invocation& IndexServer::prep_dynamic(std::vector<std::string> data, pqxx::prepare::invocation& inv)
{
    for(auto data_val : data)
        inv(data_val);
    return inv;
}


std::vector<std::string> IndexServer::getDocInfo(int url_id) {
	pqxx::work txn(*C);
	C->prepare("get_url","SELECT url,tdscore,docscore FROM docs_en WHERE id = $1");
	pqxx::result r = txn.prepared("get_url")(url_id).exec();
	txn.commit();
	const pqxx::field u = r.back()[0];
	const pqxx::field q = r.back()[1];
	const pqxx::field s = r.back()[2];
	std::vector<std::string> docinfo;
	docinfo.push_back(pqxx::to_string(u));
	docinfo.push_back(pqxx::to_string(q));
	docinfo.push_back(pqxx::to_string(s));
	return docinfo;
}

std::map<std::string,std::vector<int>> IndexServer::getTermPositions(int url_id, std::vector<std::string> terms) {
	std::string termstr;
	pqxx::work txn(*C);
	for (std::vector<std::string>::const_iterator it = terms.begin(); it != terms.end(); ++it) {
		termstr += txn.quote(*it);
		if (std::next(it) != terms.end()) {
			termstr += ",";
		}
	}
	C->prepare("get_positions","SELECT key, value FROM docs_en d, jsonb_each_text(d.segmented_grams->'unigrams') docterms WHERE d.id=$1 AND docterms.key IN ($2)");
	pqxx::result r = txn.prepared("get_positions")(url_id)(termstr).exec();
	txn.commit();
	std::map<std::string,std::vector<int>> term_positions;
	for (pqxx::result::const_iterator row = r.begin(); row != r.end(); ++row) {
		const pqxx::field term = (row)[0];
		const pqxx::field position = (row)[1];
		std::string term_(term.c_str());

		std::vector<int> positions;
		stringstream sterm(position.c_str());

		std::string p;
		while (getline(sterm, p, ','))
		{
			positions.push_back(atoi(p.c_str()));
		}
		term_positions[term_] = positions;
	}
	return term_positions;
}

/*
 * Retrieval function populate the query.
 * This all looks pretty efficient to me, but what do I know.
 * Would be great to get review / rewrite.
 */
void IndexServer::addQueryCandidates(Query::Node &query, IndexServer *indexServer, std::vector<Frag::Item> &candidates) {

	std::cout << "index_server.cc : add query candidates" << std::endl;
	if (query.leafNodes.size()==0) {
		std::string converted;
		query.term.term.toUTF8String(converted);
		std::cout << "index_server.cc - looking for " << converted << std::endl;

		phmap::parallel_flat_hash_map<std::string, std::vector<Frag::Item>>::const_iterator urls = indexServer->unigramurls_map.find(converted);
		if (urls != indexServer->unigramurls_map.end()) {
			// std::cout << "index_server.cc Found " << converted << std::endl;
			// std::cout << "index_server.cc Debug " << urls->first << std::endl;

			std::vector<Frag::Item>::const_iterator bit = urls->second.begin();
			std::vector<Frag::Item>::const_iterator eit;
			if (urls->second.size() > MAX_CANDIDATES_COUNT) {
				eit = urls->second.begin() + MAX_CANDIDATES_COUNT;
			} else {
				eit = urls->second.end();
			}
			candidates=std::vector(bit,eit);
		}
	} else {
		std::vector<Frag::Item> node_candidates;
		for (std::vector<Query::Node>::iterator it = query.leafNodes.begin() ; it != query.leafNodes.end(); ++it) {
			bool isStopWord = false;
			for (std::vector<std::pair<Query::Modifier, Query::AttributeValue>>::iterator mit = it->term.mods.begin() ; mit != it->term.mods.end(); ++mit) {
				if (mit->first == Query::Modifier::STOPWORD) {
					if (mit->second.b == true) {
						isStopWord = true;
					}
				}
			}
			if (isStopWord==true) {
				std::cout << " this is an en stopword - continue" << std::endl;
				continue;
			}
			std::vector<Frag::Item> candidates_;
			addQueryCandidates(*it, indexServer, candidates_);
			if (node_candidates.empty()) {
				node_candidates=candidates_;
			} else {
				std::vector<Frag::Item> new_candidates;
				// AND/OR is counter intuitive, AND means intersect of results while OR is union.
				if (query.op==Query::Operator::AND) {
					for (std::vector<Frag::Item>::const_iterator tit = candidates_.begin(); tit != candidates_.end(); ++tit) {
						// introduce AND , OR logic here.
						auto ait = find_if(node_candidates.begin(), node_candidates.end(), [tit](const Frag::Item r) {
							return r.url_id == tit->url_id;
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
				} else if (query.op==Query::Operator::OR) {
					for (std::vector<Frag::Item>::const_iterator tit = candidates_.begin(); tit != candidates_.end(); ++tit) {
						// introduce AND , OR logic here.
						auto ait = find_if(node_candidates.begin(), node_candidates.end(), [tit](const Frag::Item r) {
							return r.url_id == tit->url_id;
						});
						if (ait != node_candidates.end()) {
							ait->weight = ait->weight + tit->weight;
						} else {
							node_candidates.push_back(*tit);
						}
					}
				} else {
					continue;
				}
			}
		}
		candidates = node_candidates;
	}
}

bool isAdjacent (int i) { return ((i%2)==1); }
