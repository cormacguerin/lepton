#ifndef _FRAG_H_
#define _FRAG_H_

#include <unicode/unistr.h>
#include <string>
#include <set>
#include <map>
#include <vector>
#include <memory>
#include <iostream>
#include <sstream>
#include <fstream>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/istreamwrapper.h>
#include "parallel_hashmap/phmap.h"
#include <pqxx/pqxx>
#include <pqxx/strconv.hxx>
#include <sys/mman.h>
#include <sys/stat.h>

// some notes

class Frag {

    public:
        enum Type { UNIGRAM=0, BIGRAM=1, TRIGRAM=2 };
        Type prefix_type;

        Frag(Type type, int _frag_id, int _fragment_id=0, std::string p="", bool _s=false);
        ~Frag();

        int frag_id;
        int fragment_id;
        int MAX_CANDIDATES = 10000;
        std::string path;
        std::string lang;
        std::string filename;
        bool is_serving;

        /*
         * TODO : remove doc_id
         * the url id stored also in the map container so we have duplication in a critical area
         */
        struct Item {
            int doc_id;
            float tf;
            float weight;
            int no_positions;
            int positions[20];
        };
        size_t size();
        void addToIndex(phmap::parallel_flat_hash_map<std::string, int[3]> &index, std::mutex &m);
        //pqxx::prepare::invocation& prep_dynamic(std::vector<std::string> data, pqxx::prepare::invocation& inv);
        void write();
        void writeIndex();
        void remove();
        void purgeDocs(std::map<int,std::string> purge_docs);
        void insert(std::string s, std::map<int,Frag::Item> m);
        void update(std::string s, std::map<int,Frag::Item> m);
        void addWeights(int num_docs, std::string database, std::string lang);
        std::map<std::string, std::map<int, Frag::Item>> frag_map;
        std::vector<Frag::Item> getItems(int start, int end);

    private:
        std::mutex m_frag_map;
        std::mutex m_frag_mem_map;
        
        typedef std::pair<int,Frag::Item> itempair;
        Frag::Item *frag_mem_map;
        int *frag_mem_map_pos;
        std::vector<std::string> getItemKeys();
        int getTime();

        void load();
        void serializeJSON(rapidjson::Document &serialized_frag);
        void writeJsonFrag();
        void writeRawFrag();
        void writeRawMemMapFrag();
        void loadMmap();
        void loadJsonFrag();
        void loadRawFrag();

};

#endif
