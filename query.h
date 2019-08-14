
#ifndef _QUERY_H_
#define _QUERY_H_

#include <string>
#include <set>
#include <map>
#include <vector>
#include <memory>

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

		enum Operator { AND=0, OR=1, NOT=2, RESTRICT=3 };
		enum Type { ORIGINAL=0, SYNONYM=1, CONCEPT=2 };
		enum Modifier { LITERAL=0, CONFIDENCE=1, COLLECTION=2 };
		enum Flag { SYNCONF=0 };
		enum AttrType { DOUBLE=0, STRING=1 };

		//TODO:  move to std::variant 
		struct AttributeValue {
			bool b;
			std::string s;
			double d;
		};

		struct Term {
			Type type;
			std::string term;
			std::map<Modifier, std::map<Flag, AttributeValue>> mods;
			double idf;
		};

		struct Node {
			bool root;
			Operator op;
			std::vector<Term> term;
			std::string raw_query;
			double weight;

			const bool serialize() {
			}
			const bool deserialize() {
			}
			std::vector<Node> childNodes;
		};

	public:

		Query(bool root, std::string raw_query);
		~Query();

};

#endif
