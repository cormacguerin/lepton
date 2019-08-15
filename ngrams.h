
#ifndef _NGRAMS_H_
#define _NGRAMS_H_

#include <string>
#include <vector>
#include <map>
#include <unicode/ures.h>
#include <unicode/unistr.h>
#include <unicode/resbund.h>
#include <unicode/ustdio.h>
#include <unicode/putil.h>
#include <unicode/msgfmt.h>
#include <unicode/calendar.h>
#include <unicode/brkiter.h>
#include <unicode/uniset.h>
#include "texttools.h"

class Ngrams {
	private:

		std::string str_in;
		std::map<std::string,std::vector<int>> gramPositions;
		std::vector<UnicodeString> grams;
		std::vector<std::string> uni_spec;
		std::vector<std::string> ascii_spec;
		std::vector<std::string> ja_stop_words;
		std::vector<std::string> en_stop_words;


	public:
	Ngrams();
	~Ngrams();

};

#endif

