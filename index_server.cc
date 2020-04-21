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
#include <chrono>


using namespace std;
using namespace pqxx;


IndexServer::IndexServer(std::string database, std::string table)
{
	q = 0;
	x = 0;
  db = database;
  tb = table;
  // todo, you need to enable service here
	init();
}

IndexServer::~IndexServer()
{
}

void IndexServer::init() {
	try {
		C = new pqxx::connection("dbname = " + db + " user = postgres password = kPwFWfYAsyRGZ6IomXLCypWqbmyAbK+gnKIW437QLjw= hostaddr = 127.0.0.1 port = 5432");
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
  for (std::vector<std::string>::iterator lit = langs.begin(); lit != langs.end(); lit++) {
	  for (const string &ng : ngrams) {
      unigramurls_map[*lit] = phmap::parallel_flat_hash_map<std::string, std::vector<Frag::Item>>();
			loadIndex(ng, *lit);
		}
	}
}

void IndexServer::loadIndex(std::string ng, std::string lang) {

	time_t beforeload = getTime();
	std::cout << "loading index " << lang << " ... (this might take a while)." << std::endl;


	std::vector<std::string> index_files;

	struct dirent *entry;
	DIR *dp;

  std::string path = "index/" + db + "/" + tb + "/";
  std::cout << path << std::endl;
  std::replace(path.begin(),path.end(),' ','_');

	dp = opendir(path.c_str());
	if (dp == NULL) {
		perror("opendir");
		std::cout << "frag_manager.cc : Error , unable to load last frag" << std::endl;;
		exit;
	}
	std::string ext = ".frag.00001";
	while (entry = readdir(dp)) {
		std::string e_(entry->d_name);
    if (e_.substr(0,2).compare(lang) == 0) {
		  if ((e_.find(ext) != std::string::npos)) {
        if (e_.substr(e_.length()-11).compare(ext) == 0) {
          index_files.push_back(entry->d_name);
        }
      }
		}
	}
	closedir(dp);

	std::sort(index_files.begin(),index_files.end());
	if (index_files.empty()) {
		std::cout << "no index files" << std::endl;
		return;
	} else {
		std::cout << "unigramurls_map.size() " << unigramurls_map[lang].size() << std::endl;
		for (std::vector<std::string>::iterator it = index_files.begin() ; it != index_files.end(); ++it) {
			std::cout << *it << std::endl;
			int frag_id = stoi((*it).substr((*it).find('.')-5,(*it).find('.')));
			Frag frag(Frag::Type::UNIGRAM, frag_id, 1, path + lang);
			frag.addToIndex(unigramurls_map[lang]);
		}
		std::cout << "unigramurls_map["<<lang<<"].size() " << unigramurls_map[lang].size() << std::endl;
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
	time_t afterload = getTime();
	double seconds = difftime(afterload, beforeload);
	std::cout << "index_server.cc finished loading " << ng << "gram " << lang << " index in " << seconds << " seconds." << std::endl;
	/*
	for (std::unordered_map<std::string, std::vector<int>>::iterator it = ngramurls_map.begin() ; it != ngramurls_map.end(); ++it) {
		std::cout << "index_server.cc :"  << it->first << ":" << std::endl;
	}
	*/
}

void IndexServer::execute(std::string lang, std::string parsed_query, std::promise<std::string> promiseObj) {

	time_t beforeload = getTime();

	std::thread th(search, lang, parsed_query, std::move(promiseObj), this, queryBuilder);
	th.join();

	time_t afterload = getTime();
	double seconds = difftime(afterload, beforeload);
	std::cout << "index_server.cc search " << parsed_query << " executed in " << seconds << " miliseconds." << std::endl;
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
	time_t beforeload = indexServer->getTime();
	std::vector<Frag::Item> candidates;
	indexServer->addQueryCandidates(query, indexServer, candidates);
	time_t afterload = indexServer->getTime();
	double seconds = difftime(afterload, beforeload);
	std::cout << "index_server.cc gatherd " << candidates.size() << " candidates for " << parsed_query << " in " << seconds << " miliseconds." << std::endl;

	std::vector<std::string> terms = query.getTerms();

	beforeload = indexServer->getTime();
	Result result = indexServer->getResult(terms, candidates);
	result.query = query;
	afterload =indexServer->getTime();
	seconds = difftime(afterload, beforeload);
	std::cout << "index_server.cc getResult " << parsed_query << " completed in " << seconds << " miliseconds." << std::endl;
	std::cout << "result size " << result.items.size() << std::endl;
	
	// cScorer.score(&result);
	beforeload = indexServer->getTime();
	std::sort(result.items.begin(), result.items.end(),
		[](const Result::Item& l, const Result::Item& r) {
		return l.score > r.score;
	});
	if (result.items.size() > 50) {
		result.items.resize(50);
	}
	afterload = indexServer->getTime();
	seconds = difftime(afterload, beforeload);
	std::cout << "index_server.cc sort and resize " << parsed_query << " completed in " << seconds << " miliseconds." << std::endl;

	beforeload = indexServer->getTime();
	indexServer->getResultInfo(result);
	afterload = indexServer->getTime();
	seconds = difftime(afterload, beforeload);
	std::cout << "index_server.cc getResultInfo " << parsed_query << " completed in " << seconds << " miliseconds." << std::endl;

	promiseObj.set_value(result.serialize());
}

/*
 * Get the snippet, metadata, entities etc.
 */
void IndexServer::getResultInfo(Result& result) {
	/*
	if (result.items.empty()) {
		return;
	}

	x++;

	std::map<int,int> c_map;
	std::string prepstr ="(";
	for (std::vector<Result::Item>::iterator tit = result.items.begin(); tit != result.items.end(); ++tit) {
		prepstr += std::to_string(tit->url_id);
		if (std::next(tit) != result.items.end()) {
			prepstr += ",";
		}
		c_map[tit->url_id] = std::distance(result.items.begin(), tit);
	}
	prepstr += ")";

	pqxx::work txn(*C);
	C->prepare("get_docinfo_deep"+x, "SELECT id, entities, (WITH S AS (SELECT jsonb_array_elements_text(segmented_grams->'raw_text') AS snippet FROM docs_en WHERE id=D.id OFFSET 50 ROWS LIMIT 100) SELECT string_agg(snippet, ' ') FROM S) FROM docs_en AS D WHERE id IN " + prepstr);
	pqxx::result r = txn.prepared("get_docinfo_deep"+x).exec();
	txn.commit();

	for (pqxx::result::const_iterator row = r.begin(); row != r.end(); ++row) {
		const pqxx::field i = (row)[0];
		const pqxx::field t = (row)[1];
		const pqxx::field s = (row)[2];
		int id = c_map.at(atoi(i.c_str()));
		std::string entities = std::string(t.c_str());
		std::string snippet = std::string(s.c_str());

		result.items.at(id).entities = entities;
		result.items.at(id).snippet = snippet;
	}
*/
	if (result.items.empty()) {
		return;
	}

	pqxx::work txn(*C);
	C->prepare("get_docinfo_deep", "SELECT id, entities, (WITH S AS (SELECT jsonb_array_elements_text(segmented_grams->'raw_text') AS snippet FROM " + tb + "WHERE id=D.id OFFSET 50 ROWS LIMIT 100) SELECT string_agg(snippet, ' ') FROM S) FROM " + tb + " AS D WHERE id = $1");
	for (std::vector<Result::Item>::iterator tit = result.items.begin(); tit != result.items.end(); ++tit) {
		pqxx::result r = txn.prepared("get_docinfo_deep")(tit->url_id).exec();
		const pqxx::field i = r.back()[0];
		const pqxx::field t = r.back()[1];
		const pqxx::field s = r.back()[2];

		std::string entities = std::string(t.c_str());
		std::string snippet = std::string(s.c_str());
		tit->entities = entities;
		tit->snippet = snippet;
	}

	txn.commit();

}


Result IndexServer::getResult(std::vector<std::string> terms, std::vector<Frag::Item> candidates) {

	Result result;

	if (candidates.empty()) {
		return result;
	}

	if (terms.empty()) {
		return result;
	}

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

	if (prepstr_ == "()") {
		std::cout << "somehow no url ids in candidates.. corrupted index/database? " << std::endl;
		return result;
	}

	C->prepare("get_docinfo"+q,"SELECT id, url, tdscore, docscore, key, value FROM docs_en d, jsonb_each_text(d.segmented_grams->'unigrams') docterms WHERE d.id IN " + prepstr_ + " AND docterms.key IN " + prepstr);

	std::map<std::string,std::vector<int>> term_positions;
	std::vector<pqxx::result> pqxx_results;

	pqxx::prepare::invocation w_invocation = txn.prepared("get_docinfo"+q);
	prep_dynamic(terms, w_invocation);
	pqxx::result r = w_invocation.exec();

	time_t beforeload = getTime();
	time_t getResultTime = 0;

	int id = 0;
	int last_id = 0;
	for (pqxx::result::const_iterator row = r.begin(); row != r.end(); ++row) {

		time_t beforeload_ = getTime();

		const pqxx::field i = (row)[0];
		const pqxx::field u = (row)[1];
		const pqxx::field q = (row)[2];
		const pqxx::field s = (row)[3];
		const pqxx::field term = (row)[4];
		const pqxx::field position = (row)[5];

		id = c_map.at(atoi(i.c_str()));
		result.items.at(id).score = 1;
		result.items.at(id).url = pqxx::to_string(u);
		result.items.at(id).docscore = atof(s.c_str());
		result.items.at(id).score = atof(s.c_str()) * result.items.at(id).weight;

		//
		if (id != last_id && row!=r.begin()) {
			double wscore = 1.0;
			if (result.items.at(last_id).terms.size() > 1) {
				// a distance variable that increases per stopword(as we don't store stopword posisions)
				int d = 1;
				std::string term;
				for (std::vector<std::string>::const_iterator s = terms.begin(); s != terms.end(); ++s) {
					if (std::next(s) == terms.end()) {
						break;
					}
					if (*s  != "__SW__") {
						term = *s;
					}
					if (*std::next(s)  == "__SW__") {
						d++;
						continue;
					}
					std::map<std::string,std::vector<int>>::iterator xit = result.items.at(last_id).terms.find(term);
					std::map<std::string,std::vector<int>>::iterator yit = result.items.at(last_id).terms.find(*(std::next(s)));
					if (xit!=result.items.at(last_id).terms.end() && yit!=result.items.at(last_id).terms.end()) {
						for (std::vector<int>::iterator zit = result.items.at(last_id).terms.at(term).begin(); zit != result.items.at(last_id).terms.at(term).end(); zit++) {
							int c = std::count(result.items.at(last_id).terms.at(*(std::next(s))).begin(), result.items.at(last_id).terms.at(*(std::next(s))).end(), (*zit)+d);
							// if there is no score add a demote. why?
							// say you search for massachusetts institute of technology
							// you will get a lot of promotions for instute of technology
							// eg. tokyo instutute of technology. this might be unwanted.
							if (c == 0) {
								wscore = wscore * 0.5;
							} else {
								wscore += std::count(result.items.at(last_id).terms.at(*(std::next(s))).begin(), result.items.at(last_id).terms.at(*(std::next(s))).end(), (*zit)+d);
							}
						}
					}
					// wscore += wp;
					d = 1;
				}
				wscore = sqrt(wscore);
			}
			result.items.at(last_id).wscore = wscore;
			result.items.at(last_id).score = result.items.at(last_id).score * wscore;

			time_t afterload_ = getTime();
			double seconds = difftime(afterload_, beforeload_);
			getResultTime += seconds;
			std::cout << "index_server.cc : result  " << last_id << " processed in " << seconds << std::endl;
		}
		last_id = id;

		std::string term_(term.c_str());

		std::vector<int> positions;
		stringstream sterm(position.c_str());

		std::string p;
		while (getline(sterm, p, ',')) {
			positions.push_back(atoi(p.c_str()));
		}
		result.items.at(id).terms[term_] = positions;
	}
	std::cout << "index_server.cc : getResult  processed in " << getResultTime << std::endl;
	txn.commit();
	time_t afterload = getTime();
	double seconds = difftime(afterload, beforeload);
	std::cout << "index_server.cc : results time : " << seconds << std::endl;
	
	beforeload = getTime();
	txn.commit();
	afterload = getTime();
	seconds = difftime(afterload, beforeload);
	std::cout << "index_server.cc : commit time : " << seconds << std::endl;

	// if this is the next url, the process the previous one testing for
	// adjecent terms in multi terms queries and prioritize if necessary
	/*
	for (std::vector<Result::Item>::iterator it = result.items.begin(); it != result.items.end(); ++it) {
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
						w += std::count(it->terms.at(*(std::next(s))).begin(), it->terms.at(*(std::next(s))).end(), (*zit)+1);
					}
					it->score += log(w);
				}
			}
		}
	}
	*/

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
	/*
	for (phmap::parallel_flat_hash_map<std::string, std::vector<Frag::Item>>::const_iterator it = unigramurls_map.begin(); it != unigramurls_map.end(); it++) {
		for (std::vector<Frag::Item>::const_iterator vit = (it->second).begin() ; vit != (it->second).end(); ++vit) {
			std::cout << " map " << it->first << " : " << (*vit).url_id << std::endl;
		}
	}
	*/

	std::cout << "index_server.cc : add query candidates" << std::endl;
	if (query.leafNodes.empty()) {
		std::string converted;
		query.term.term.toUTF8String(converted);
		std::cout << "index_server.cc - looking for " << converted << std::endl;

		phmap::parallel_flat_hash_map<std::string, std::vector<Frag::Item>>::const_iterator urls = indexServer->unigramurls_map[query.lang].find(converted);
		if (urls != indexServer->unigramurls_map[query.lang].end()) {
			std::cout << "index_server.cc Found " << urls->first << std::endl;
			/*
			for (std::vector<Frag::Item>::const_iterator it = (urls->second).begin() ; it != (urls->second).end(); ++it) {
				std::cout << "index_server.cc - at " << (urls->second).begin() - it << " : " << it->url_id << std::endl;
			}
			*/
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
				std::cout << "index_server.cc this is an en stopword - continue" << std::endl;
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
							// std::cout << "index_server.cc add candidate " << ait->url_id << std::endl;
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

int IndexServer::getTime() {
	return std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::system_clock::now().time_since_epoch()
	).count();
}

