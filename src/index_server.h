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
#include "frag.h"
#include "result.h"
#include "parallel_hashmap/phmap.h"
#include "segmenter.h"
#include "pg_pool.h"

class IndexServer {

    public:
        IndexServer(std::string database, std::string table);
        ~IndexServer();
        Segmenter seg;
        std::string status;
        std::mutex m;
        std::mutex pm;
        void init();
        void run();
        void stop();
        void addQueryCandidates(Query::Node &query, IndexServer *indexServer, std::vector<Result::Item> &candidates);
        void execute(std::string lang, std::string type, std::string query, std::string columns, std::string filter, std::string pages, std::promise<std::string> promiseObj);
        static void search(std::string lang, std::string parsed_query, std::string columns, std::string filter, std::string pages, std::promise<std::string> promiseObj, IndexServer *indexServer, QueryBuilder queryParser);
        static void suggest(std::string lang, std::string parsed_query, std::promise<std::string> promiseObj, IndexServer *indexServer);
        std::vector<std::string> langs = {"en","ja","zh","ko","es","de","fr"};
        std::map<std::string,int> getServingInfo();
        std::map<std::string,int> getPercentLoaded();
        std::string getServingStatus();
        bool do_run;
        std::string separateGram(const char* c, bool isCJK);

    private:
        PgPool pgPool;
        pqxx::connection* C;
        // pqxx::work* txn;
        // a map of memory mapped fragments which we can query
        std::map<int,std::unique_ptr<Frag>> mmapped_frags;

        // a map of langages to a parallel flat hash map of terms(words) to fragments (url id and weight) 
        std::map<std::string,phmap::parallel_flat_hash_map<std::string, int[3]>> unigramurls_map;
        std::map<std::string,phmap::parallel_flat_hash_map<std::string, int[3]>> bigramurls_map;
        std::map<std::string,phmap::parallel_flat_hash_map<std::string, int[3]>> trigramurls_map;
        std::map<std::string,int> percent_loaded;
        QueryBuilder queryBuilder;
        // a map of languages to a map of strings (partial words)) to a map of suggestions (the int is the number of occurrences in the corpus)
        std::map<std::string, std::map<std::string, std::vector<std::pair<std::string,int>>>> suggestions;
        std::string db;
        std::string tb;
        // std::vector<std::string> getDocInfo(int doc_id);
        // std::map<std::string,std::vector<int>> getTermPositions(int doc_id, std::vector<std::string> terms);
        Result getResult(std::vector<std::string> terms, std::vector<Result::Item> candidates);
        void doFilter(std::string filter, std::vector<Result::Item> &candidates, bool has_query);
        void getResultInfo(Result& result, std::vector<std::string> terms, std::string columns, std::string lang);
        // pqxx::prepare::invocation& prep_dynamic(std::vector<std::string> data, pqxx::prepare::invocation& inv);
        void loadIndex(Frag::Type type, std::string lang);
        void buildSuggestions(std::string lang);
        void addSuggestion(std::string term, std::string lang, int count);
        void getStopSuggest();
        const int MAX_CANDIDATES_COUNT = 1000;
        int getTime();
};

#endif

