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

		void serialize_(rapidjson::Document &serialized_shard);
		void write();
		void updateShard();
		size_t size();
		void insert(std::string s, std::map<int,Shard::Term> m);

		rapidjson::Document serializeTerm(Term t);

};

#endif
