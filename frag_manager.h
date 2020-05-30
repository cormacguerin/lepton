
#ifndef _FRAG_MANAGER_H_
#define _FRAG_MANAGER_H_

//#include <unicode/ures.h>
#include <unicode/unistr.h>
//#include <unicode/resbund.h>
//#include <unicode/ustdio.h>
//#include <unicode/putil.h>
#include <string>
#include <set>
#include <map>
#include <unordered_map>
#include "parallel_hashmap/phmap.h"
#include <vector>
#include <memory>
#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include "frag.h"


class FragManager {
    private:
        Frag::Type frag_type;
        std::string database;
        std::string table;
        std::string lang;
        // hash map of term strings to frag ids
        // std::map<std::string, int> gram_frag_term_index;
        phmap::parallel_flat_hash_map<std::string, int> gram_frag_term_index;
        // hash map of term strings to a map of doc ids term data
        std::map<std::string, std::map<int, Frag::Item>> grams_terms;
        std::string path;
        // base number of terms per frag
        //	int FRAG_SIZE=100000;
        // x number of terms per subsequent frag
        // the logic is that the first frag will have all the common terms
        // it will ballon, while subsequent fragments will have less occurences
        // eg. 
        // frag 1 has 50 terms
        // frag 2 has 100 terms and so on
        int FRAG_SIZE_MULTIPLIER=100;
        int BATCH_SIZE=100000;
        void loadFrags();
        void saveFrags();
        void loadFragIndex();
        std::string readFile(std::string filename);
        std::vector<std::string> getFiles(std::string path, std::string ext);
        std::vector<int> indices;
        int last_frag_id;

    public:

        FragManager(Frag::Type type, std::string db, std::string tb, std::string l);
        ~FragManager();

        std::map<int,std::unique_ptr<Frag>> frags;
        void addTerms(std::map<std::string, Frag::Item> doc_grams);
        void syncFrags();
        void mergeFrags(int num_docs, std::string database);

};

#endif
