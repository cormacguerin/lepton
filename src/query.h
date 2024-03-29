
#ifndef _QUERY_H_
#define _QUERY_H_

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
#include "frag.h"

// some notes
// sorting is done by idf -ln(probability)
// google has a noperm concept, where incorrect order of matched words should be demoted.
// it seems to me that in this implementation we can ignore that as we will naturally promote phrases instead.
// we need to add confidence scores at some point, for both synonyms and concepts

// In this implimentation there is no explicit PHRASE Term or WORD Operator
// Instead everything can be a phrase, which are generically captured as a Term.
// A term could be a word or a phrase, words or phrases would also be considered concepts equally.


class Query {
	private:

	public:

		enum Operator { TERM=0, AND=1, OR=2, NOT=3, RESTRICT=4 };
		enum Type { ORIGINAL=0, SYNONYM=1, CONCEPT=2 };
        enum Gram { UNIGRAM=0, BIGRAM=1, TRIGRAM=2, NGRAM=3 };
		enum Modifier { LITERAL=0, CONFIDENCE=1, COLLECTION=2, SYNCONF=3, STOPWORD=4 };

//		static const std::string OperatorList[4];
//		static const std::string TypeList[3];
//		static const std::string ModifierList[5];

		//TODO:  move to std::variant 
		struct AttributeValue {
			bool b;
			std::string s;
			double d;
		};

		struct Term {
			Type type;
            Gram gram;
			icu::UnicodeString term;
			std::vector<std::pair<Modifier, AttributeValue>> mods;
		};

		struct Node {
			bool root;
			Operator op;
			Query::Term term;
			std::string raw_query;
			std::string lang;

			std::string serialize();
			void serialize_(rapidjson::Document &serialized_query);
			void deserialize();

			std::vector<std::string> getTerms();
			void getTerms_(std::vector<std::string> &terms);

			void updateQuery();
			std::vector<Node> leafNodes;
		};

		Query(bool root, std::string raw_query);
		~Query();

};

#endif
