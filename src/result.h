
#ifndef _RESULT_H_
#define _RESULT_H_

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
#include "query.h"

// some notes
// sorting is done by idf -ln(probability)
// google has a noperm concept, where incorrect order of matched words should be demoted.
// it seems to me that in this implementation we can ignore that as we will naturally promote phrases instead.
// we need to add confidence scores at some point, for both synonyms and concepts

// In this implimentation there is no explicit PHRASE Item or WORD Operator
// Instead everything can be a phrase, which are generically captured as a Item.
// A item could be a word or a phrase, words or phrases would also be considered concepts equally.


class Result {
	private:

	public:

		enum Operator { AND=0, OR=1, NOT=2, RESTRICT=3 };
		enum Type { ORIGINAL=0, SYNONYM=1, CONCEPT=2 };
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

		struct Item {
			Type type;
			icu::UnicodeString item;
			std::map<Modifier, AttributeValue> mods;
			double tf;
			double weight;
			double tdscore;
			double docscore;
			double score;
			double wscore;
			int url_id;
			std::string lang;
			std::string url;
			std::string snippet;
			std::string entities;
			std::map<std::string,std::vector<int>> terms;
			void updateResult();
		};

		const std::string serialize();
		// return the query for debugging purposes.
		Query::Node query;

		std::vector<Result::Item> items;

		Result();
		~Result();

};

#endif
