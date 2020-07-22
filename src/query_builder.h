#ifndef _QUERY_BUILDER_H_
#define _QUERY_BUILDER_H_

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
#include <future>
#include "query.h"

/*
 * a helper to parse a query similar to how we index in segmenter.h
 */

class QueryBuilder {
	private:
		std::string query_str;
		std::vector<std::string> uni_spec;
		std::vector<std::string> ascii_spec;
		std::vector<std::string> ja_stop_words;
		std::vector<std::string> en_stop_words;

		// trim from start (in place)
		static inline void ltrim(std::string &s) {
			s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
				return !std::isspace(ch);
			}));
		}

		// trim from end (in place)
		static inline void rtrim(std::string &s) {
			s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
				return !std::isspace(ch);
			}).base(), s.end());
		}

		// trim from both ends (in place)
		static inline void trimInPlace(std::string &s) {
				ltrim(s);
				rtrim(s);
		}

        int findPattern(std::vector<int> pattern, std::vector<int> data);
        Query::Node genTermNode(std::string lang, Query::Term term, std::string query_str);
        void buildNgramNode(std::string lang, Query::Node branchNode, Query::Node &ngramNode, std::vector<int> &stoppattern);

        // ngram patterns
        std::vector<std::vector<int>> ngram_patterns = {
          { 0,0,0,0,0 },
          { 0,0,0,0 },
          { 0,0,0 },
          { 0,0 },
          { 0,1,1,0 },
          { 0,1,0 }
        };


	public:
		QueryBuilder();
		~QueryBuilder();
		void init();
		void build(std::string lang, std::string query_str, Query::Node &result);
};

#endif

