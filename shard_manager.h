
#ifndef _SHARD_MANAGER_H_
#define _SHARD_MANAGER_H_

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
#include "shard.h"


class ShardManager {
	private:
		// hash map of term strings to shard ids
		// std::map<std::string, int> unigram_shard_term_index;
		phmap::parallel_flat_hash_map<std::string, int> unigram_shard_term_index;
		std::map<std::string, int> bigram_shard_term_index;
		std::map<std::string, int> trigram_shard_term_index;
		// hash map of term strings to a map of doc ids term data
		phmap::flat_hash_map<std::string, std::map<int, Shard::Term>> unigram_terms;
		phmap::flat_hash_map<std::string, std::map<int, Shard::Term>> bigram_terms;
		phmap::flat_hash_map<std::string, std::map<int, Shard::Term>> trigram_terms;
		int SHARD_SIZE=10000;
		int BATCH_SIZE=1000000;
		void loadLastShard();

	public:

		ShardManager();
		~ShardManager();

		std::unique_ptr<Shard> last_shard;
		void addTerms(std::map<std::string, Shard::Term> doc_unigrams,
			std::map<std::string, Shard::Term> doc_bigrams, 
			std::map<std::string, Shard::Term> doc_trigrams);
		void syncShards();

};

#endif
