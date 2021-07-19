#include "index_server.h"
#include <unistd.h>
#include "base64.h"
#include <algorithm>
#include <ctime>
#include "query.h"
#include "texttools.h"
#include "dirent.h"
#include <chrono>
#include <mutex>
#include <cstdio>
#include "util.h"



using namespace std;
using namespace pqxx;


IndexServer::IndexServer(std::string database, std::string table)
{
  _q_ = 0;
  db = database;
  tb = table;
  init();
}

IndexServer::~IndexServer()
{
  delete C;
}

void IndexServer::init() {
  auto config = getConfig();
  try {
    C = new pqxx::connection("dbname = " + db + " user = " + config.postgres_user + " password = " + config.postgres_password + " hostaddr = " + config.postgres_host + " port = " + config.postgres_port);
    if (C->is_open()) {
      cout << "Opened database successfully: " << C->dbname() << endl;
    } else {
      cout << "Can't open database" << endl;
    }
  } catch (const std::exception &e) {
    status = "failed";
    cerr << e.what() << std::endl;
  }
  C->prepare("get_stop_suggest", "SELECT stop,lang,gram,idf FROM stop_suggest ORDER BY lang, stop, idf DESC;");
  status = "loading";
  seg.init("","");
  //std::string ngrams[] = {"uni","bi","tri"};
  //std::string langs[] = {"en","ja","zh"};
}

void IndexServer::run() {
  do_run = true;
  while (do_run) {
    for (std::vector<std::string>::iterator lit = langs.begin(); lit != langs.end(); lit++) {
      //std::cout << "lit " << *lit << std::endl;
      if (softMutexLock(m)==true) {
        if (unigramurls_map.find(*lit) == unigramurls_map.end()) {
          unigramurls_map[*lit] = phmap::parallel_flat_hash_map<std::string, std::vector<Frag::Item>>();
        }
        m.unlock();
      } else {
        continue;
      }
      //std::cout << "loadIndex " << *lit << std::endl;
      loadIndex(Frag::Type::UNIGRAM, *lit);
      loadIndex(Frag::Type::BIGRAM, *lit);
      loadIndex(Frag::Type::TRIGRAM, *lit);
      buildSuggestions(*lit);
    }
    usleep(6000000);
    // we handle suggestions starting with stopwords separately.
    getStopSuggest();
    status = "serving";
  }
  m.lock();
  unigramurls_map.clear();
  m.unlock();
  status = "shutdown";
}

void IndexServer::stop() {
  do_run = false;
  unigramurls_map.empty();
  bigramurls_map.empty();
  trigramurls_map.empty();
  suggestions.empty();
}

/*
 * load the index files for the specified ngram and language into the map
 */
void IndexServer::loadIndex(Frag::Type type, std::string lang) {
  std::string ng;
  if (type == Frag::Type::UNIGRAM) {
    ng = "uni";
  }
  if (type == Frag::Type::BIGRAM) {
    ng = "bi";
  }
  if (type == Frag::Type::TRIGRAM) {
    ng = "tri";
  }

  time_t beforeload = getTime();
  std::cout << "loading index " << lang << " ... (this might take a while)." << std::endl;

  std::vector<std::string> index_files;

  struct dirent *entry;
  DIR *dp;

  std::string path = "index/" + db + "/" + tb + "/";
  //std::cout << "loadIndex " << path << std::endl;
  std::replace(path.begin(),path.end(),' ','_');

  dp = opendir(path.c_str());
  if (dp == NULL) {
    perror("opendir");
    std::cout << "frag_manager.cc : Error , unable to load last frag" << std::endl;;
    status = "failed";
    return;
  }
  std::string ext = ".frag.00001";
  while (entry = readdir(dp)) {
    std::string e_(entry->d_name);
    if (e_.substr(0,2).compare(lang) == 0) {
      if (e_.substr(3,ng.length()).compare(ng) == 0) {
        if ((e_.find(ext) != std::string::npos)) {
          if (e_.substr(e_.length()-11).compare(ext) == 0) {
            index_files.push_back(entry->d_name);
          }
        }
      }
    }
  }
  closedir(dp);

  double counter = 0;
  std::sort(index_files.begin(),index_files.end());
  if (index_files.empty()) {
    std::cout << "no index files" << std::endl;
    status = "noindex";
    return;
  } else {
    // std::cout << "unigramurls_map.size() " << unigramurls_map[lang].size() << std::endl;
    for (std::vector<std::string>::iterator it = index_files.begin(); it != index_files.end(); ++it) {
      std::cout << *it << std::endl;
      int frag_id = stoi((*it).substr((*it).find('.')-5,(*it).find('.')));
      Frag frag(type, frag_id, 1, path + lang);
      frag.addToIndex(unigramurls_map[lang], m);
      percent_loaded[lang]=std::ceil((counter++/index_files.size())*100);
      std::cout << counter << " percent_loaded " << lang << " " << percent_loaded[lang] << std::endl;
    }
  }

  /*
     pqxx::work txn(*C);
     C->prepare("load_"+ng+"gram_"+lang+"_urls_batch", "SELECT "+ng+"grams_"+lang+".gram, array_agg(doc_id)::int[] FROM (SELECT gram_id, doc_id, weight FROM docunigrams_en ORDER BY score) AS dng INNER JOIN "+ng+"grams_"+lang+" ON ("+ng+"grams_"+lang+".id = dng.gram_id) GROUP BY "+ng+"grams_"+lang+".gram");

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
  // std::cout << "index_server.cc finished loading " << ng << "gram " << lang << " index in " << seconds << " seconds." << std::endl;
  /*
     for (std::unordered_map<std::string, std::vector<int>>::iterator it = ngramurls_map.begin() ; it != ngramurls_map.end(); ++it) {
     std::cout << "index_server.cc :"  << it->first << ":" << std::endl;
     }
     */
}

void IndexServer::execute(std::string lang, std::string type, std::string parsed_query, std::string columns, std::string filter, std::string pages, std::promise<std::string> promiseObj) {

  time_t beforeload = getTime();

  if (type == "search") {
    std::thread t(search, lang, parsed_query, columns, filter, pages, std::move(promiseObj), this, queryBuilder);
    t.detach();
  } else if (type == "suggest") {
    std::thread t(suggest, lang, parsed_query, std::move(promiseObj), this);
    t.detach();
  }

  time_t afterload = getTime();
  double seconds = difftime(afterload, beforeload);
  std::cout << lang << std::endl;
  std::cout << " unigramurls_map[query.lang].size " <<  unigramurls_map[lang].size() << std::endl;
  std::cout << "index_server.cc search " << parsed_query << " executed in " << seconds << " miliseconds." << std::endl;
}

/*
 * We do 4 disk (db) calls here
 *  - doFilter, where we read jsonb filters for each candidate doc
 *  - applyAcl, where we read jsonb ACL filters for each candidate doc
 *  - getResults, where we get word positions and apply base score
 *  - getResultInfo, where we get all information for a set of results (20 by default)
 *  TODO
 *  - make the above faster by either doing the first 3 in one db call or using an SS table
 */
void IndexServer::search(std::string lang, std::string parsed_query, std::string columns, std::string filter, std::string pages, std::promise<std::string> promiseObj, IndexServer *indexServer, QueryBuilder queryBuilder) {

  Query::Node query;
  queryBuilder.build(lang, parsed_query, query);

  int page_num = 0;
  int page_result_num = 20;
  double total_seconds = 0.0;

  rapidjson::Document pagination;
  try {
    pagination.Parse(pages.c_str());
  } catch (const std::exception &e) {
    std::cout << "ERROR" << std::endl;
    cerr << e.what() << std::endl;
    return;
  }

  std::cout << "pages" << std::endl;
  std::cout << pages << std::endl;

  if ( pagination.IsObject() == 1) {
    std::cout << "pages is object" << std::endl;
    for (rapidjson::Value::ConstMemberIterator it = pagination.MemberBegin(); it != pagination.MemberEnd(); ++it) {

      if (strcmp(it->name.GetString(),"page_number")==0) {
        if ( it->value.IsInt() == 1) {
          page_num = it->value.GetInt();
        }
      }

      if (strcmp(it->name.GetString(),"page_result_count")==0) {
        if ( it->value.IsInt() == 1) {
          page_result_num = it->value.GetInt();
        }
      }

    }
  }

  // TODO update query with other stuff.
  // - 
  // Also in a massively scaled out version these would all be separate services with their own network stack
  // The query would run to various servelets getting rewritten and then results retrieved and scored.
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
  std::cout << "index_server.cc gathered " << candidates.size() << " candidates for " << parsed_query << " in " << seconds << " miliseconds." << std::endl;
  total_seconds += seconds;

  // new
  std::sort(candidates.begin(), candidates.end(),
      [](const Frag::Item& l, const Frag::Item& r) {
      return l.weight > r.weight;
      });
  std::vector<Frag::Item>::const_iterator bit = candidates.begin();
  std::vector<Frag::Item>::const_iterator eit;
  if (candidates.size() > indexServer->MAX_CANDIDATES_COUNT) {
    eit = candidates.begin() + indexServer->MAX_CANDIDATES_COUNT;
  } else {
    eit = candidates.end();
  }
  candidates=std::vector<Frag::Item>(bit, eit);

  // filter the candidates against supplied filters
  // TODO ACLs can be done in the same way.
  beforeload = indexServer->getTime();
  if (parsed_query == "") {
    indexServer->doFilter(filter, candidates, false);
  } else if (candidates.size() > 0) {
    indexServer->doFilter(filter, candidates, true);
  } else {
    Result result;
    result.query = query;
    result.result_count = candidates.size();
    promiseObj.set_value(result.serialize());
    return;
  }
  
  afterload = indexServer->getTime();
  seconds = difftime(afterload, beforeload);
  std::cout << "index_server.cc " << candidates.size() << " candidates after filtering in " << seconds << " miliseconds." << std::endl;
  total_seconds += seconds;

  // get and score the documents from the backend.
  std::vector<std::string> terms = query.getTerms();
  beforeload = indexServer->getTime();
  Result result = indexServer->getResult(terms, candidates);
  result.query = query;
  result.result_count = candidates.size();
  result.page_num = page_num;
  result.page_result_num = page_result_num;
  afterload = indexServer->getTime();
  seconds = difftime(afterload, beforeload);
  std::cout << "index_server.cc getResult " << parsed_query << " completed in " << seconds << " miliseconds." << std::endl;
  std::cout << "result size " << result.items.size() << std::endl;
  total_seconds += seconds;

  // order the first 20 documents according to score 
  // TODO add pagination
  beforeload = indexServer->getTime();

  std::sort(result.items.begin(), result.items.end(),
      [](const Result::Item& l, const Result::Item& r) {
      return l.score > r.score;
      });
  std::cout << "page_num " << page_num << std::endl;
  std::cout << "page_result_num " << page_result_num << std::endl;
  if (result.items.size() >= page_result_num * page_num) {
    result.items.erase(result.items.begin(), result.items.begin() + page_result_num * page_num);
  }
  if (result.items.size() > page_result_num) {
    result.items.resize(page_result_num);
    // result.items.erase(result.items.begin() + page_result_num, result.items.end());
  }

  afterload = indexServer->getTime();
  seconds = difftime(afterload, beforeload);
  std::cout << "index_server.cc sort and resize " << parsed_query << " completed in " << seconds << " miliseconds." << std::endl;
  total_seconds += seconds;

  beforeload = indexServer->getTime();
  indexServer->getResultInfo(result,terms,columns,lang);
  afterload = indexServer->getTime();
  seconds = difftime(afterload, beforeload);
  std::cout << "index_server.cc getResultInfo " << parsed_query << " completed in " << seconds << " miliseconds." << std::endl;
  total_seconds += seconds;

  result.query_time = total_seconds;

  promiseObj.set_value(result.serialize());
}

/*
 * similar to above but we scan for potential suggestions.
 */
void IndexServer::suggest(std::string lang, std::string parsed_query, std::promise<std::string> promiseObj, IndexServer *indexServer) {

  time_t beforeload = indexServer->getTime();
  time_t afterload = indexServer->getTime();
  double seconds = difftime(afterload, beforeload);
  
  /*
  for (std::map<std::string, std::vector<std::pair<std::string,int>>>::const_iterator ssit = indexServer->suggestions[lang].begin(); ssit != indexServer->suggestions[lang].end(); ssit++) {
    std::cout << ssit->first << std::endl;
  }
  */
  
  std::map<std::string, std::vector<std::pair<std::string,int>>>::const_iterator sit = indexServer->suggestions[lang].find(indexServer->seg.segmentTerm(parsed_query,lang));
  if (sit != indexServer->suggestions[lang].end()) {
    rapidjson::Document suggest_response;
    suggest_response.Parse("{}");
    rapidjson::Document::AllocatorType& allocator = suggest_response.GetAllocator();
    rapidjson::Value suggest_array(rapidjson::kArrayType);
    for (std::vector<std::pair<std::string,int>>::const_iterator it = sit->second.begin(); it != sit->second.end(); it++) {
      //std::cout << it->first << std::endl;
      suggest_array.PushBack(rapidjson::Value(const_cast<char*>(it->first.c_str()), allocator).Move(), allocator);
    }
    suggest_response.AddMember("suggestions", rapidjson::Value(suggest_array, allocator).Move(), allocator);
    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    suggest_response.Accept(writer);
    promiseObj.set_value((std::string)buffer.GetString());
  } else {
    promiseObj.set_value("");
  }
}

/*
 * build suggestions
 */
void IndexServer::buildSuggestions(std::string lang) {
  int j = 0;
  std::cout << "unigramurls_map[lang].size() " << unigramurls_map[lang].size() << std::endl;
  for (phmap::parallel_flat_hash_map<std::string, std::vector<Frag::Item>>::const_iterator urls = unigramurls_map[lang].begin(); urls != unigramurls_map[lang].end(); urls++) {
    //std::cout << "tb" << tb << " - " << urls->first << " " << urls->second.size() << std::endl;
    if (urls->second.size() > 1) {
      addSuggestion(urls->first, lang, urls->second.size());
    }
    j++;
    if (j%1000 == 0) {
      std::cout << j << " loading suggestions " << ((float)j*100/unigramurls_map[lang].size()) << " % " << std::endl;
    }
  }
}

void IndexServer::addSuggestion(std::string term, std::string lang, int count) {
  for (int i=1; i<=term.length(); i++) {
    std::string sugg = (term).substr(0,i);
    if (suggestions[lang][sugg].size() == 0) {
      suggestions[lang][sugg].push_back(std::pair<std::string,int>(term,count));
    } else {
      int index = 0;
      int position = -1;
      bool found = false;
      std::pair<std::string,int> value;
      for (std::vector<std::pair<std::string,int>>::iterator it = suggestions[lang][sugg].begin(); it != suggestions[lang][sugg].end(); it++) {
        if (it->first == term) {
          found = true;
          position = -1;
          it->second = count;
          break;
        }  
        if (it->second < count) {
          if (position == -1) {
            position = index;
          }
        } 
        index++;
      }
      if (position != -1) {
        suggestions[lang][sugg].insert(suggestions[lang][sugg].begin()+position, std::pair<std::string,int>(term,count));
        if (suggestions[lang][sugg].size() == 11) {
          suggestions[lang][sugg].pop_back();
        }
      } else if (suggestions[lang][sugg].size() < 10 && found == false) {
        suggestions[lang][sugg].push_back(std::pair<std::string,int>(term,count));
      }
    }
    //if (suggestions[lang][sugg].size() == 0) {
    //}
  }
}

/*
 * Get the snippet, metadata, entities etc.
 * the code works well engough but it pretty unintelligable if someone can rewrite it sometime
 */
void IndexServer::getResultInfo(Result& result, std::vector<std::string> terms, std::string user_columns, std::string lang) {
  if (result.items.empty()) {
    return;
  }
  int snippet_size = 50;

  // TODO parse columns to make sure it's legit
  std::string columns = "id,raw_text";
  if (user_columns.length() > 0) {
    columns += ",";
    columns += user_columns;
  }

 // C->prepare("get_docinfo_deep", "SELECT " + columns + " FROM \"" + tb + "\" WHERE id = $1");

  for (std::vector<Result::Item>::iterator rit = result.items.begin(); rit != result.items.end(); ++rit) {
    /*
       for (std::map<std::string,std::vector<int>>::iterator tit__ = rit->terms.begin(); tit__ != rit->terms.end(); tit__++) {
       for (std::vector<int>::iterator tit___ = tit__->second.begin(); tit___ != tit__->second.end(); tit___++) {
       std::cout << tit__->first << " " << *tit___ << std::endl;
       }
       }
    */
    std::map<int,int> best_match;
    int position = 0;
    int tophits = 0;
    if (terms.size() > 1) {
      for (std::vector<std::string>::iterator tit = terms.begin(); tit != terms.end()-1; ++tit) {
        std::map<std::string,std::vector<int>>::iterator tit_ = rit->terms.find(*tit);
        if (tit_ != rit->terms.end()) {
          for (std::vector<int>::iterator xit = rit->terms.at(*tit).begin(); xit != rit->terms.at(*tit).end(); ++xit) {
            std::map<std::string,std::vector<int>>::iterator tit__ = rit->terms.find(*std::next(tit));
            if (tit__ != rit->terms.end()) {
              for (std::vector<int>::iterator nit = rit->terms.at(*std::next(tit)).begin(); nit != rit->terms.at(*std::next(tit)).end(); ++nit) {
                if (*xit == *nit-1) {
                  best_match[*xit] +=3;
                } else if (*xit > *nit && *xit < *nit+50) {
                  best_match[*xit]++;
                }
                if (best_match[*xit] > tophits) {
                  tophits = best_match[*xit];
                  position = *xit;
                }
              }
            } else {
              std::cout << "index_server.cc : WARNING doc_ud " << rit->doc_id << " has no terms for query " << terms[0] << std::endl;
              continue;
            }
          }
        }
      }
    } else if (terms.size() == 1) {
      // there is only one term so get the first occurrence of the term as the snippet position.
      // this could be better, we should look for meaningful prose..
      std::map<std::string,std::vector<int>>::iterator tit__ = rit->terms.find(terms[0]);
      if (tit__ != rit->terms.end()) {
        if (tit__->second.size()>0) {
          position = tit__->second.at(0);
        } else {
          std::cout << "index_server.cc : WARNING doc_id " << rit->doc_id << " has no positions for term " << terms[0] << std::endl;
        }
      } else {
        std::cout << "index_server.cc : WARNING doc_ud " << rit->doc_id << " has no terms for query " << terms[0] << std::endl;
        continue;
      }
    }
    /*
    // std::cout << "position " << " " << rit->doc_id << " " << position << std::endl;
    // std::cout << "tophits " << tophits << std::endl;
    // std::cout << "position " << tophits << std::endl;
    for (std::map<int,int>::iterator it = best_match.begin(); it != best_match.end(); ++it) {
    std::cout << "best match " << it->first << " : " << it->second << std::endl;
    }
    */

    pm.lock();
    pqxx::work txn(*C);
    pqxx::result r;
    try {
      r = txn.exec_params("SELECT " + columns + " FROM \"" + tb + "\" WHERE id = $1",rit->doc_id);
    } catch (const std::exception &e) {
      std::cout << "ERROR" << std::endl;
      cerr << e.what() << std::endl;
      return;
    }
    txn.commit();
    pm.unlock();

    for (int i=0; i < r.columns(); i++) {
      std::string column_name = std::string(r.column_name(i));
      const pqxx::field v = r.back()[i];
      if (v.is_null() == false) {
        if (column_name == "raw_text") {
          rit->snippet = seg.getSnippet(std::string(v.c_str()),lang,position);
        } else {
          rit->data[column_name] = std::string(v.c_str());
        }
      }
    }
  }

}

void IndexServer::doFilter(std::string filter, std::vector<Frag::Item> &candidates, bool has_query) {

  if (filter == "") {
    return;
  }

  std::cout << "filter " << filter << std::endl;

  std::string prepstr_="";
  for (std::vector<Frag::Item>::const_iterator tit = candidates.begin(); tit != candidates.end(); ++tit) {
    tit->doc_id;
    prepstr_ += std::to_string(tit->doc_id);
    if (std::next(tit) != candidates.end()) {
      prepstr_ += ",";
    }
  }

  std::string prep_filter = "";
  rapidjson::Document parsed_filter;
  try {
    parsed_filter.Parse(filter.c_str());
  } catch (const std::exception &e) {
    std::cout << "ERROR" << std::endl;
    cerr << e.what() << std::endl;
    return;
  }

  /*
    std::cout << "prepstr " << prepstr_ << std::endl;
    std::cout << "filter " << filter << std::endl;
    std::cout << parsed_filter.IsArray() << std::endl;;
    std::cout << parsed_filter.IsObject() << std::endl;;
    std::cout << "--" << std::endl;;
  */

  if ( parsed_filter.IsObject() == 1) {

    std::string pgq;
    std::string key;
    std::string value;
    std::string op;
    for (rapidjson::Value::ConstMemberIterator it = parsed_filter.MemberBegin(); it != parsed_filter.MemberEnd(); ++it) {

      if (strcmp(it->name.GetString(),"key")==0) {
        if ( it->value.IsString() == 1) {
          key = it->value.GetString();
        }
      }

      if (strcmp(it->name.GetString(),"value")==0) {
        if ( it->value.IsString() == 1) {
          value = it->value.GetString();
        }
      }

      if (strcmp(it->name.GetString(),"operator")==0) {
        if ( it->value.IsString() == 1) {
          op = it->value.GetString();
        }
      }
    }

    if (key == "") {
      return;
    }
    if (value == "") {
      return;
    }

    if (op == "equals") {
      pgq = " AND metadata->>'" + key + "' = '" + value + "'";
    } else if (op == "contains") {
      pgq = " AND metadata->>'" + key + "' LIKE '%" + value + "%'";
    } else if (op == "greater_than") {
      pgq = " AND (metadata->>'" + key + "') > '" + value + "'";
    } else if (op == "less_than") {
      pgq = " AND (metadata->>'" + key + "') < '" + value + "'";
    } else {
      return;
    }
    prep_filter += pgq;

  } else if ( parsed_filter.IsArray() == 1) {

    for (rapidjson::Value::ConstValueIterator itr = parsed_filter.Begin(); itr != parsed_filter.End(); ++itr) {

      const rapidjson::Value& obj = *itr;

      std::string pgq;
      std::string key;
      std::string value;
      std::string op;

      for (rapidjson::Value::ConstMemberIterator it = obj.MemberBegin(); it != obj.MemberEnd(); ++it) {

        if (strcmp(it->name.GetString(),"key")==0) {
          if (it->value.IsString()) {
            key = it->value.GetString();
          } else {
            std::cout << "KEY IS NOT STRING CONTINUEa" << std::endl;
            continue;
          }
        }
        std::cout << "key " << key << std::endl;

        if (strcmp(it->name.GetString(),"value")==0) {
          if (it->value.IsString()) {
            value = it->value.GetString();
          } else if (it->value.IsInt()) {
            value = std::to_string(it->value.GetInt());
          } else if (it->value.IsInt64()) {
            value = std::to_string((long)it->value.GetInt64());
          } else if (it->value.IsUint()) {
            value = std::to_string((unsigned long)it->value.GetUint());
          } else if (it->value.IsUint64()) {
            value = std::to_string((unsigned long)it->value.GetUint64());
          } else if (it->value.IsDouble()) {
            value = std::to_string((long)it->value.GetDouble());
          }
        }

        if (strcmp(it->name.GetString(),"operator")==0) {
          op = it->value.GetString();
        }
      }

      if (key == "") {
        std::cout << "NO KEY CONTINUE" << std::endl;
        continue;
      }
      if (value == "") {
        std::cout << "NO VALUE CONTINUE" << std::endl;
        continue;
      }

      std::cout << "operator " << op << std::endl;

      if (op == "equals") {
        pgq = " AND metadata->>'" + key + "' = '" + value + "'";
      } else if (op == "contains") {
        pgq = " AND metadata->>'" + key + "' LIKE '%" + value + "%'";
      } else if (op == "greater_than") {
        pgq = " AND (metadata->>'" + key + "') > '" + value + "'";
      } else if (op == "less_than") {
        pgq = " AND (metadata->>'" + key + "') < '" + value + "'";
      } else {
        continue;
      }
      prep_filter += pgq;

    }

  } else {
    return;
  }

  std::string filter_query = "";
  if (has_query) {
    filter_query = "SELECT DISTINCT(id) FROM \"" + tb + "\" d, jsonb_each_text(d.metadata) metadata WHERE d.id IN (" + prepstr_ + ") " + prep_filter;
  } else if (prep_filter.length() > 10) {
    prep_filter.erase(0,4);
    filter_query = "SELECT DISTINCT(id) FROM \"" + tb + "\" d, jsonb_each_text(d.metadata) metadata WHERE " + prep_filter;
  } else {
    return;
  }
  std::cout << filter_query << std::endl;

  // std::string prepared_filter = base64_encode(reinterpret_cast<const unsigned char*>(filter_query.c_str()),filter_query.length());
  std::string prepared_filter = std::to_string(std::hash<std::string>{}(filter_query));
  std::cout << "prepared_filter " << prepared_filter << std::endl;

  pm.lock();
  pqxx::work txn(*C);
  C->prepare(prepared_filter,filter_query);
  pqxx::result r = txn.exec_prepared(prepared_filter);
  C->unprepare(prepared_filter);
  txn.commit();
  pm.unlock();

  int num = 0;
  for (pqxx::result::const_iterator row = r.begin(); row != r.end(); ++row) {

    const pqxx::field i = (row)[0];
    int id = atoi(pqxx::to_string(i).c_str());
    // std::cout << "index_server.cc doFilter id " << id << std::endl;

    if (has_query) {
      std::vector<Frag::Item>::iterator fit = std::find_if(
          candidates.begin(),
          candidates.end(),
          [id](const Frag::Item& f) { return f.doc_id == id; }
          );
      if (fit != candidates.end() && num < candidates.size()) {
        // std::cout << num << " index_server.cc doFilter id " << fit->doc_id << " found." << std::endl;
        std::swap(candidates.at(num), *fit);
        num++;
      }
    } else {
      Frag::Item F;
      F.doc_id = id;
      F.weight = 1.0;
      F.tf = 1.0;
      candidates.push_back(F);
    }
  }

  std::cout << "index_server.cc filter size " << r.size() << std::endl;
  std::cout << "index_server.cc candidates.size " << candidates.size() << std::endl;

  if (has_query) {
    candidates.resize(num);
  }
}

Result IndexServer::getResult(std::vector<std::string> terms, std::vector<Frag::Item> candidates) {

  Result result;

  if (candidates.empty()) {
    return result;
  }

  /*
  if (terms.empty()) {
    return result;
  }
  */

  _q_++;
  int p = 0;
  std::string prepstr="(";
  std::string termsstr="";
  std::vector<std::string> prepterms;
  for (std::vector<std::string>::const_iterator it = terms.begin(); it != terms.end(); ++it) {
    p++;
    prepstr += "$" + std::to_string(p);
    prepterms.push_back(*it);
    termsstr += *it;
    if (std::next(it) != terms.end()) {
      prepstr += ",";
      termsstr += ",";
    }
  }
  prepstr += ")";

  std::map<int,int> c_map;
  int x = 0;
  std::string prepstr_="";
  for (std::vector<Frag::Item>::const_iterator tit = candidates.begin(); tit != candidates.end(); ++tit) {
    Result::Item item;
    item.tf = tit->tf;
    item.weight = tit->weight;
    item.doc_id = tit->doc_id;
    result.items.push_back(item);
    prepstr_ += std::to_string(tit->doc_id);
    if (std::next(tit) != candidates.end()) {
      prepstr_ += ",";
    }
    c_map[item.doc_id]=x;
    x++;
  }

  std::map<std::string,std::vector<int>> term_positions;
  std::vector<pqxx::result> pqxx_results;

  // sql timing
  time_t beforeload = getTime();
  time_t getResultTime = 0;
  std::string statement;


  if (terms.size() > 0) {
    statement = "SELECT id, url, tdscore, docscore, key, value FROM \"" + tb + "\" d, jsonb_each_text(d.segmented_grams->'unigrams') docterms WHERE d.id IN (" + prepstr_ + ") AND docterms.key IN " + prepstr;
    std::cout << statement << std::endl;
    std::cout << termsstr << std::endl;
  } else {
    statement = "SELECT id, url, tdscore, docscore, key, value FROM \"" + tb + "\" d, jsonb_each_text(d.segmented_grams->'unigrams') docterms WHERE d.id IN (" + prepstr_ + ")";
  }

  pm.lock();
  pqxx::work txn(*C);

  pqxx::result r = txn.exec_params(statement, pqxx::prepare::make_dynamic_params(prepterms));
  txn.commit();
  pm.unlock();

  time_t afterload = getTime();
  double seconds = difftime(afterload, beforeload);
  std::cout << "index_server.cc : sql for getResult processed in " << seconds << std::endl;

  beforeload = getTime();
  getResultTime = 0;

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

    if (!c_map.count(atoi(i.c_str()))) {
      continue;
    } 

    id = c_map.at(atoi(i.c_str()));
    result.items.at(id).score = 1;
    result.items.at(id).url = pqxx::to_string(u);
    result.items.at(id).docscore = atof(s.c_str());
    result.items.at(id).score = atof(s.c_str()) * result.items.at(id).weight;

    if (id != last_id && row!=r.begin()) {
      double wscore = 1.0;
      if (result.items.at(last_id).terms.size() > 1) {
        // a distance variable that increases per stopword(as we don't store stopword positions)
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
  afterload = getTime();
  seconds = difftime(afterload, beforeload);
  std::cout << "index_server.cc : results time : " << seconds << std::endl;

  beforeload = getTime();
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

/*
pqxx::prepare::invocation& IndexServer::prep_dynamic(std::vector<std::string> data, pqxx::prepare::invocation& inv)
{
  for(auto data_val : data)
    inv(data_val);
  return inv;
}
*/

/*
std::vector<std::string> IndexServer::getDocInfo(int doc_id) {
  pqxx::work txn(*C);
  C->prepare("get_url","SELECT url,tdscore,docscore FROM \"" + tb + "\" WHERE id = $1");
  pqxx::result r = txn.prepared("get_url")(doc_id).exec();
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

std::map<std::string,std::vector<int>> IndexServer::getTermPositions(int doc_id, std::vector<std::string> terms) {
  std::string termstr;
  pqxx::work txn(*C);
  for (std::vector<std::string>::const_iterator it = terms.begin(); it != terms.end(); ++it) {
    termstr += txn.quote(*it);
    if (std::next(it) != terms.end()) {
      termstr += ",";
    }
  }
  C->prepare("get_positions","SELECT key, value FROM docs_en d, jsonb_each_text(d.segmented_grams->'unigrams') docterms WHERE d.id=$1 AND docterms.key IN ($2)");
  pqxx::result r = txn.prepared("get_positions")(doc_id)(termstr).exec();
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
*/

/*
 * Retrieval function populate the query.
 * This all looks pretty efficient to me, but what do I know.
 * Would be great to get review / rewrite.
 */
void IndexServer::addQueryCandidates(Query::Node &query, IndexServer *indexServer, std::vector<Frag::Item> &candidates) {
  /*
     for (phmap::parallel_flat_hash_map<std::string, std::vector<Frag::Item>>::const_iterator it = unigramurls_map.begin(); it != unigramurls_map.end(); it++) {
     for (std::vector<Frag::Item>::const_iterator vit = (it->second).begin() ; vit != (it->second).end(); ++vit) {
     std::cout << " map " << it->first << " : " << (*vit).doc_id << std::endl;
     }
     }
     */

  std::cout << "index_server.cc : add query candidates" << std::endl;
  if (query.leafNodes.empty()) {
    std::string converted;
    query.term.term.toUTF8String(converted);
    std::cout << "index_server.cc - looking for \'" << converted << "\'" << std::endl;

    // hard lock here, we want to always honor incoming queries.
    // TODO separately we will need to DOS protections
    m.lock();		
    phmap::parallel_flat_hash_map<std::string, std::vector<Frag::Item>>::const_iterator urls = unigramurls_map[query.lang].find(converted);
    m.unlock();
    if (urls != unigramurls_map[query.lang].end()) {
      std::cout << "index_server.cc Found " << urls->second.size() << " candidates for " << urls->first << std::endl;

      /*
         for (std::vector<Frag::Item>::const_iterator it = (urls->second).begin() ; it != (urls->second).end(); ++it) {
         std::cout << "index_server.cc - at " << (urls->second).begin() - it << " : " << it->doc_id << std::endl;
         }
         */

      std::vector<Frag::Item>::const_iterator bit = urls->second.begin();
      std::vector<Frag::Item>::const_iterator eit;
      if (urls->second.size() > MAX_CANDIDATES_COUNT*3) {
        eit = urls->second.begin() + MAX_CANDIDATES_COUNT*3;
      } else {
        eit = urls->second.end();
      }
      candidates=std::vector<Frag::Item>(bit,eit);
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
      addQueryCandidates(*it, this, candidates_);
      if (node_candidates.empty()) {
        node_candidates=candidates_;
      } else {
        std::vector<Frag::Item> new_candidates;
        // AND/OR is counter intuitive, AND means intersect of results while OR is union.
        if (query.op==Query::Operator::AND) {
          std::cout << "DEBUG AND " << candidates_.size() << std::endl;
          for (std::vector<Frag::Item>::const_iterator tit = candidates_.begin(); tit != candidates_.end(); ++tit) {
            // introduce AND , OR logic here.
            auto ait = find_if(node_candidates.begin(), node_candidates.end(), [tit](const Frag::Item r) {
                return r.doc_id == tit->doc_id;
                });
            if (ait != node_candidates.end()) {
              // std::cout << "index_server.cc add candidate " << ait->doc_id << std::endl;
              ait->weight=ait->weight + tit->weight;
              new_candidates.push_back(*ait);
            }
          }
          if (new_candidates.empty()) {
            node_candidates.clear();
          } else {
            auto the_end = query.leafNodes.end();
            --the_end;
            //                        if (it != the_end) {
            std::cout << "DEBUG - not the end" << std::endl;
            node_candidates = new_candidates;
            //                        } else {
            std::cout << "DEBUG - the end" << std::endl;
            //                        }
          }
        } else if (query.op==Query::Operator::OR) {
          std::cout << "DEBUG OR " << candidates_.size() << std::endl;
          for (std::vector<Frag::Item>::const_iterator tit = candidates_.begin(); tit != candidates_.end(); ++tit) {
            // introduce AND , OR logic here.
            auto ait = find_if(node_candidates.begin(), node_candidates.end(), [tit](const Frag::Item r) {
                return r.doc_id == tit->doc_id;
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

void IndexServer::getStopSuggest() {
  std::cout << " getStopSuggest " << std::endl;

  pm.lock();
  pqxx::work txn(*C);
  pqxx::result r = txn.exec_prepared("get_stop_suggest");
  txn.commit();
  pm.unlock();

  for (pqxx::result::const_iterator row = r.begin(); row != r.end(); ++row) {
    const pqxx::field stop = (row)[0];
    const pqxx::field lang = (row)[1];
    const pqxx::field gram = (row)[2];
    const pqxx::field idf = (row)[3];
    std::string stop_(stop.c_str());
    std::string lang_(lang.c_str());
    std::string gram_(gram.c_str());
    // fake count but whatever.
    int x  = (int)(atof(idf.c_str())*10000);
    addSuggestion(gram_,lang_,x);
  }
}

bool isAdjacent (int i) { return ((i%2)==1); }

int IndexServer::getTime() {
  return std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::system_clock::now().time_since_epoch()
      ).count();
}

std::map<std::string,int> IndexServer::getServingInfo() {
  std::map<std::string,int> lang_term_count;
  std::cout << "getServingInfo try lock" << std::endl;
  if (softMutexLock(m)==true) {
    std::cout << "getServingInfo lock success" << std::endl;
    for (std::map<std::string,phmap::parallel_flat_hash_map<std::string, std::vector<Frag::Item>>>::const_iterator it = unigramurls_map.begin(); it != unigramurls_map.end(); it++) {
      std::cout << "getServingInfo : " << it->first << " " << it->second.size() << std::endl;
      lang_term_count[it->first] = it->second.size();
    }
    std::cout << "getServingInfo unlock" << std::endl;
    // softMutexLock locks, so make sure to unlock before returning.
    m.unlock();
    std::cout << "getServingInfo return success" << std::endl;
    return lang_term_count;
  } else {
    std::cout << "getServingInfo return fail" << std::endl;
    return lang_term_count;
  }
}

std::map<std::string,int> IndexServer::getPercentLoaded() {
  std::map<std::string,int> pl;
  std::cout << "getPercentLoaded try lock" << std::endl;
  if (softMutexLock(m)==true) {
    std::cout << "getPercentLoaded lock success" << std::endl;
    pl = percent_loaded;
    std::cout << "getPercentLoaded unlock" << std::endl;
    m.unlock();
    std::cout << "getPercentLoaded return success" << std::endl;
    return pl;
  } else {
    std::cout << "getPercentLoaded return fail" << std::endl;
    return pl;
  }
}

std::string IndexServer::getServingStatus() {
  return status;
}

