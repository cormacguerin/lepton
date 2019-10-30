
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
		// hash map of term strings to frag ids
		// std::map<std::string, int> gram_frag_term_index;
		phmap::parallel_flat_hash_map<std::string, int> gram_frag_term_index;
		// hash map of term strings to a map of doc ids term data
		std::map<std::string, std::map<int, Frag::Item>> grams_terms;
		int FRAG_SIZE=10000;
		int BATCH_SIZE=100000;
		void loadLastFrag();
		void loadFrags();
		void saveFrags();
		void loadFragIndex();
		std::string readFile(std::string filename);
		std::vector<std::string> getFiles(std::string path, std::string ext);
		int last_frag_id;

	public:

		FragManager(Frag::Type type);
		~FragManager();

		std::map<int,std::unique_ptr<Frag>> frags;
		void addTerms(std::map<std::string, Frag::Item> doc_grams);
		void syncFrags();
		void mergeFrags(int num_docs, std::string lang);

};

#endif
