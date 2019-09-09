
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
#include <vector>
#include <memory>
#include <iostream>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include "shard.h"

// some notes
// sorting is done by idf -ln(probability)
// google has a noperm concept, where incorrect order of matched words should be demoted.
// it seems to me that in this implementation we can ignore that as we will naturally promote phrases instead.
// we need to add confidence scores at some point, for both synonyms and concepts

// In this implimentation there is no explicit PHRASE Term or WORD Operator
// Instead everything can be a phrase, which are generically captured as a Term.
// A term could be a word or a phrase, words or phrases would also be considered concepts equally.


class ShardManager {
	private:

	public:

		ShardManager();
		~ShardManager();

		void addTerms(std::map<std::string, Shard::Term> doc_unigram_map,
						 std::map<std::string, Shard::Term> doc_bigram_map, 
						 std::map<std::string, Shard::Term> doc_trigram_map);

};

#endif
