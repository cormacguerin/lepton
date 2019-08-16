#ifndef _QUERY_PARSER_H_
#define _QUERY_PARSER_H_

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

/*
 * a helper to parse a query similar to how we index in segmenter.h
 */

class QueryParser {
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

	public:
		QueryParser();
		~QueryParser();
		void init();
		void parse(std::string lang, std::string query_str);
		static void execute(std::string lang, std::string query, std::promise<std::string> *promiseObj);
};

#endif
