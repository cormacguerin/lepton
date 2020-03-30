#ifndef _FRAG_H_
#define _FRAG_H_

#include <unicode/unistr.h>
#include <string>
#include <set>
#include <map>
#include <vector>
#include <memory>
#include <iostream>
#include <sstream>
#include <fstream>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/istreamwrapper.h>
#include "parallel_hashmap/phmap.h"

// some notes

class Frag {

	public:
		enum Type { UNIGRAM=0, BIGRAM=1, TRIGRAM=2 };
		Type prefix_type;

		Frag(Type type, int _frag_id, int _fragment_id=0, std::string p="");
		~Frag();

		int frag_id;
		int fragment_id;
    std::string path;

		struct Item {
			int url_id;
			double tf;
			double weight;
		};
		typedef std::pair<int,Frag::Item> itempair;
		std::map<std::string, std::map<int, Frag::Item>> frag_map;
		std::vector<std::string> getItemKeys();

		void serializeJSON(rapidjson::Document &serialized_frag);
		void write();
		void writeJsonFrag(std::string filename);
		void writeRawFrag(std::string filename);
		void writeIndex();
		size_t size();
		void insert(std::string s, std::map<int,Frag::Item> m);
		void update(std::string s, std::map<int,Frag::Item> m);
		void addWeights(int num_docs, std::string database);
		void load();
		void loadJsonFrag(std::string filename);
		void loadRawFrag(std::string filename);
		//void addToIndex(phmap::parallel_flat_hash_map<std::string, phmap::flat_hash_map<int, Frag::Item>> &index);
		void addToIndex(phmap::parallel_flat_hash_map<std::string, std::vector<Frag::Item>> &index);
		std::string readFile(std::string filename);

	private:

};

#endif
