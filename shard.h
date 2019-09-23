#ifndef _SHARD_H_
#define _SHARD_H_

#include <unicode/unistr.h>
#include <string>
#include <set>
#include <map>
#include <vector>
#include <memory>
#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/istreamwrapper.h>
#include "parallel_hashmap/phmap.h"

// some notes

class Shard {
	private:

	public:
		enum Type { UNIGRAM=0, BIGRAM=1, TRIGRAM=2 };
		Type prefix_type;

		Shard(Type type, int shard_id);
		~Shard();

		int id;

		struct Term {
			int url_id;
			double tf;
			double weight;
		};
		std::map<std::string, std::map<int, Shard::Term>> shard_map;
		std::vector<std::string> getTermKeys();

		void serialize_(rapidjson::Document &serialized_shard);
		void write();
		size_t size();
		void insert(std::string s, std::map<int,Shard::Term> m);
		void update(std::string s, std::map<int,Shard::Term> m);
		void load(int shard_id);
		void addToIndex(phmap::parallel_flat_hash_map<std::string, std::map<int, Shard::Term>> &index);
		std::string readFile(std::string filename);

};

#endif
