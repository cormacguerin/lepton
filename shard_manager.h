
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
#include <vector>
#include <memory>
#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include "shard.h"


class ShardManager {
	private:
		// vector of shards containing the set of mapped hash terms
		//std::vector<std::set<int>> unigram_shard_term_index;
		//std::vector<std::set<int>> bigram_shard_term_index;
		//std::vector<std::set<int>> trigram_shard_term_index;
		std::unordered_map<std::string, int> unigram_shard_term_index;
		std::unordered_map<std::string, int> bigram_shard_term_index;
		std::unordered_map<std::string, int> trigram_shard_term_index;
		// hash map of map of shard url ids to terms
		std::unordered_map<std::string, std::map<int, Shard::Term>> unigram_term_index;
		std::unordered_map<std::string, std::map<int, Shard::Term>> bigram_term_index;
		std::unordered_map<std::string, std::map<int, Shard::Term>> trigram_term_index;
		int SHARD_SIZE=10000;
		void loadLatestShard();

	public:

		ShardManager();
		~ShardManager();

		void addTerms(std::map<std::string, Shard::Term> doc_unigrams,
			std::map<std::string, Shard::Term> doc_bigrams, 
			std::map<std::string, Shard::Term> doc_trigrams);
		void syncShards();

};

#endif
