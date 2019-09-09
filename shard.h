
#ifndef _SHARD_H_
#define _SHARD_H_

//#include <unicode/ures.h>
#include <unicode/unistr.h>
//#include <unicode/resbund.h>
//#include <unicode/ustdio.h>
//#include <unicode/putil.h>
#include <string>
#include <set>
#include <map>
#include <vector>
#include <memory>
#include <iostream>
#include <unordered_map>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

// some notes


class Shard {
	private:

	public:

		struct Term {
			int url_id;
			double tf;
			double weight;
		};
		std::unordered_map<std::string, std::map<int, Shard::Term>> unigramurls_map;

		int id;

		void serialize_(rapidjson::Document &serialized_shard);
		void serialize();
		void updateShard();

		rapidjson::Document serializeTerm(Term t);


		Shard();
		~Shard();

};

#endif
