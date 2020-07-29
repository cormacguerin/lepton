#ifndef _SENTENCE_PIECE_PROCESSOR_H_
#define _SENTENCE_PIECE_PROCESSOR_H_

#include <unicode/ures.h>
#include <unicode/unistr.h>
#include <unicode/resbund.h>
#include <unicode/ustdio.h>
#include <unicode/putil.h>
#include <unicode/msgfmt.h>
#include <unicode/calendar.h>
#include <unicode/brkiter.h>
#include <unicode/uniset.h>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <exception>
#include <streambuf>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <numeric>
#include <pqxx/pqxx>
#include <postgresql/libpq-fe.h>
#include "frag.h"

class Segmenter {
	private:
		std::string str_in;
		std::vector<std::string> uni_spec;
		std::vector<std::string> ascii_spec;
		std::vector<std::string> ja_stop_words;
		std::vector<std::string> en_stop_words;
		pqxx::connection* C;
		pqxx::work* txn;
		std::string concat_positions(std::vector<int> pos);
        void addSuggestCandidate(std::vector<std::string> i, double f, std::vector<std::pair<std::vector<std::string>, double>> &s);
        int SUGGEST_SIZE = 30;

	public:
		Segmenter();
		~Segmenter();
		void init(std::string database);
		void parse(std::string id, std::string lang, std::string str_in, std::string table,
				   std::map<std::string, Frag::Item> &doc_unigram_map,
				   std::map<std::string, Frag::Item> &doc_bigram_map,
				   std::map<std::string, Frag::Item> &doc_trigram_map,
                   std::map<std::vector<std::string>,double> &stopSuggest);
		void tokenize(std::string text, std::vector<std::string> *pieces);
		void detokenize(std::vector<std::string> pieces, std::string text);
        std::string getSnippet(std::string text, std::string lang, int position);
};

#endif
