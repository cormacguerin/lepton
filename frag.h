#ifndef _FRAG_H_
#define _FRAG_H_

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

class Frag {
	private:

	public:
		enum Type { UNIGRAM=0, BIGRAM=1, TRIGRAM=2 };
		Type prefix_type;

		Frag(Type type, int _frag_id, int _fragment_id=0);
		~Frag();

		int frag_id;
		int fragment_id;

		struct Item {
			int url_id;
			double tf;
			double weight;
		};
		typedef std::pair<int,Frag::Item> itempair;
		std::map<std::string, std::map<int, Frag::Item>> frag_map;
		std::vector<std::string> getItemKeys();

		void serialize_(rapidjson::Document &serialized_frag);
		void write();
		void writeIndex();
		size_t size();
		void insert(std::string s, std::map<int,Frag::Item> m);
		void update(std::string s, std::map<int,Frag::Item> m);
		void addWeights(int num_docs);
		void load();
		//void addToIndex(phmap::parallel_flat_hash_map<std::string, phmap::flat_hash_map<int, Frag::Item>> &index);
		void addToIndex(phmap::parallel_flat_hash_map<std::string, std::vector<Frag::Item>> &index);
		std::string readFile(std::string filename);

};

#endif
