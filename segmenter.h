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
#include <pqxx>

class Segmenter {
	private:
		std::string str_in;
		std::vector<std::string> uni_spec;
		std::vector<std::string> ascii_spec;
		std::vector<std::string> ja_stop_words;
		std::vector<std::string> en_stop_words;
		pqxx::connection* C;
		pqxx::work* txn;

	public:
		Segmenter();
		~Segmenter();
		void init();
		void parse(std::string lang, std::string url, std::string str_in);
		std::string update_grams_table(std::string url, std::string gram, int c);
		void tokenize(std::string text, std::vector<std::string> *pieces);
		void detokenize(std::vector<std::string> pieces, std::string text);
};

#endif
