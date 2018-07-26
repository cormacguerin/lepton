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
#include <set>
#include "texttools.h"
#include <chrono>

#define N_GRAM_SIZE 3
#define IS_CJK false

std::string str_in;
std::map<std::string,std::vector<int>> gramPositions;
std::vector<UnicodeString> grams;
std::vector<std::string> uni_spec;
std::vector<std::string> ascii_spec;
std::vector<std::string> ja_stop_words;
std::vector<std::string> en_stop_words;

/*
	std::string sanitizeText(std::string str) {
		auto isPunct = [](char c) { 
			return std::ispunct(static_cast<unsigned char>(c));
		};
		auto isDigit = [](char c) { 
			return std::isdigit(static_cast<unsigned char>(c));
		};

		// remove punctuations
		str.erase(std::remove_if(str.begin(), str.end(), isPunct), str.end());
		// remove digits
		str.erase(std::remove_if(str.begin(), str.end(), isDigit), str.end());
		return str;
	}

	// convert to lowercase
	std::string toLowerCase(std::string str) {
		std::transform(str.begin(), str.end(), str.begin(), [](char c) {
				return std::tolower(static_cast<unsigned char>(c));
				});
		return str;
	}

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
	static inline void trim(std::string &s) {
		ltrim(s);
		rtrim(s);
	}

	// If this has special characters it's probably not a good candidate so just remove.
	bool isWord(std::string str) {
		for (int i=0; i < str.length(); i++) {
			if (isdigit(str.at(i))) {
				return false;
			}
			if (ispunct(str.at(i))) {
				return false;
			}
		}
		return true;
	}
*/


int main() {

	std::ifstream ascii_spec_dict("data/ascii_special_characters.txt");
	std::ifstream uni_spec_dict("data/unicode_special_chars.txt");
	std::ifstream ja_stop_words_dict("data/japanese_stop_words.txt");
	std::ifstream en_stop_words_dict("data/english_stop_words.txt");

	std::string line;

	while (getline(ascii_spec_dict, line)) {
		try {
			ascii_spec.push_back(line);
		} catch (std::exception& e) {
			std::cout << "Exception in getUnigrams: " << e.what() << std::endl;
		}
	}

	while (getline(uni_spec_dict, line)) {
		try {
			uni_spec.push_back(line);
		} catch (std::exception& e) {
			std::cout << "Exception in getUnigrams: " << e.what() << std::endl;
		}
	}

	while (getline(ja_stop_words_dict, line)) {
		try {
			ja_stop_words.push_back(line);
		} catch (std::exception& e) {
			std::cout << "Exception in getUnigrams: " << e.what() << std::endl;
		}
	}

	while (getline(en_stop_words_dict, line)) {
		try {
			en_stop_words.push_back(line);
		} catch (std::exception& e) {
			std::cout << "Exception in getUnigrams: " << e.what() << std::endl;
		}
	}


	// temp containsers we use for processing
	std::map<std::string,std::vector<int>> gramPositions;
	std::map<std::vector<std::string>, int> gramCandidates;
	std::map<int*, int> arrGramCandidates;
	std::vector<std::string> gramWindow;
	std::vector<UnicodeString> grams;

	// this is a redis connection (were replacing this with postgres for the index)
	//	client.connect();
	// postgres connection

	std::ifstream input("input.txt");

	std::string str_in((std::istreambuf_iterator<char>(input)),
			std::istreambuf_iterator<char>());

	// convert to lowercase
	std::transform((str_in).begin(), (str_in).end(), (str_in).begin(), ::tolower);

	UnicodeString uni_str = str_in.c_str();

	UErrorCode status = U_ZERO_ERROR;
	// BreakIterator *wordIterator = BreakIterator::createWordInstance(Locale("ja","JAPAN"), status);
	BreakIterator *wordIterator = BreakIterator::createWordInstance(Locale("en","US"), status);
	// BreakIterator *wordIterator = BreakIterator::createWordInstance(Locale("hb","IL"), status);
	wordIterator->setText(uni_str);
	int32_t p = wordIterator->first();
	int32_t l = p;
	int i = 0;

	int ja_stop_words_count = 0;
	int en_stop_words_count = 0;

	while (p != BreakIterator::DONE) {
		
		// printf("Boundary at position %d\n", p);
		bool isStopWord = false;
		p = wordIterator->next();
		std::string converted;
		UnicodeString tmp = uni_str.tempSubString(l,p-l);
		tmp.toUTF8String(converted);
		l=p;
		// skip special characters
		if ( std::find(ascii_spec.begin(), ascii_spec.end(), converted) != ascii_spec.end() ) {
			continue;
		}
		if ( std::find(uni_spec.begin(), uni_spec.end(), converted) != uni_spec.end() ) {
			continue;
		}
		// insert the vector occurrence position.
		trimInPlace(converted);
		if (converted.empty()) {
			continue;
		}
		UnicodeString uc = UnicodeString::fromUTF8(converted);
		grams.push_back(uc);

//		std::cout << "gramPositions[converted].size() " << gramPositions[converted].size() << " : " << converted << std::endl;
//		std::cout << "isStopWord : " << isStopWord << std::endl;

		if (gramPositions[converted].size() == 0 && isStopWord == true) {
			i++;
			continue;
		}
		gramPositions[converted].push_back(i);
		
		i++;
	}
	
	std::cout << "INFO : no. grams found " << gramPositions.size() << std::endl;

	// 80% of total time in this loop/
	for (std::map<std::string, std::vector<int>>::iterator it = gramPositions.begin(); it != gramPositions.end(); it++) {
		// do not process for single occurrences.
		if ((it->second).size() > 1) {
			for (std::vector<int>::iterator pit = it->second.begin(); pit != it->second.end(); pit++ ) {
				std::vector<std::string> tmpgram;
				std::string converted_gram;
				UnicodeString this_gram = grams.at(*pit);
				this_gram.toUTF8String(converted_gram);
				int n = 0;
				// skip anything starting with a stop word.
				if ( std::find(ja_stop_words.begin(), ja_stop_words.end(), converted_gram) != ja_stop_words.end() ) {
					n++;
					continue;
				} else if ( std::find(en_stop_words.begin(), en_stop_words.end(), converted_gram) != en_stop_words.end() ) {
					n++;
					continue;
				}

				while (n < N_GRAM_SIZE) {
					UnicodeString tmp;
					if (*pit < grams.size() - N_GRAM_SIZE) {
						std::string converted;
						std::string converted_gram;
						UnicodeString this_gram = grams.at(*pit+n);
						this_gram.toUTF8String(converted_gram);
						// skip ngrams ending in a stopword
						tmp = grams.at(*pit+n);
						if (converted_gram.empty()) {
							n++;
							continue;
						} else {
							tmp.toUTF8String(converted);
							trimInPlace(converted);
							tmpgram.push_back(converted);
							// skip ngrams ending in a stopword (This is SLOW)
							// this is SLOW (20% of total time)
							if ( std::find(ja_stop_words.begin(), ja_stop_words.end(), converted_gram) != ja_stop_words.end() ) {
								n++;
								continue;
							} else if ( std::find(en_stop_words.begin(), en_stop_words.end(), converted_gram) != en_stop_words.end() ) {
								n++;
								continue;
							}
							// this is SLOW (20% of total time)
							gramCandidates[tmpgram]++;
						}
					}
					n++;
				}
			}
		}
	}
	
	std::cout << "INFO : no. ngrams found " << arrGramCandidates.size() << std::endl;

	// At this point we have a map of nGram candidates and a map of words/terms/unigrams
	// We perform some reduction on the gramCandidates (we only want the largest unique
	// matches)
	// eg. 3 instances of aAbBcC and we find 3 instances of each aA aAbB aAbBcC we
	// want to remove aA and aAbB if there are no separate matches for these.
	// 
	// The map of ngrams is already sorted and unique.
	// If the next entry is shorter than the current entry and the occurrence
	// count is the same then we should remove it.
	//
	// We are ok for ngrams with stop words in the middle,
	// eg. wizard of oz, 梅の花, over the moon.
	// But things starting or ending with stopwords we remove.
	// eg. the smashing pumpkins = smashing punkins.
	//     the dark tower = dark tower
	//     top of the morning to you = top of the morning
	// 
	rapidjson::Document docngrams;
	docngrams.Parse("{}");
	rapidjson::Document::AllocatorType& allocator = docngrams.GetAllocator();

	// This loop is actually really fast
	for (std::map<std::vector<std::string>, int>::iterator git = gramCandidates.begin(); git != gramCandidates.end(); git++ ) {
		if (git->second > 1) {
			if (std::next(git) != gramCandidates.end()) {
				//   std::cout << " - - - - " << std::endl;
				//   std::cout << "current " << git->first << " " << git->second << std::endl;
				//   std::cout << "next " << std::next(git)->first << " " << std::next(git)->second << std::endl;
				int nextlen = (std::next(git)->first).size();
				int currentlen = (git->first).size();
				// if the next match is longer maybe we should use that instead.
				std::string gram;
				for (auto const& s : git->first) { 
					gram += s; 
					if (IS_CJK == false) {
						gram += " ";
					}
				}
				if (nextlen >= currentlen) {
					if ((std::next(git)->first).at(currentlen-1) == (git->first).back()) {
						if ((std::next(git)->second != git->second)) {
							// the next one has less matches so this one is still valid. Add!
							// std::cout << git->first << " " << git->second << std::endl;
							rapidjson::Value k((trim(gram).c_str()), allocator);
							docngrams.AddMember(k, rapidjson::Value(git->second), allocator);
							std::cout << trim(gram) << " " << std::to_string(git->second).c_str() << std::endl;
						} else {
							// the next one is a longer maching candidate so skip this one.
							continue;
						}
					} else {
						rapidjson::Value k((trim(gram).c_str()), allocator);
						docngrams.AddMember(k, rapidjson::Value(git->second), allocator);
						std::cout << trim(gram) << " " << std::to_string(git->second).c_str() << std::endl;
					}
				} else {
					rapidjson::Value k((trim(gram).c_str()), allocator);
					docngrams.AddMember(k, rapidjson::Value(git->second), allocator);
					std::cout << trim(gram) << " " << std::to_string(git->second).c_str() << std::endl;
				}
			}
		}
	}
	
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	docngrams.Accept(writer);

	//std::cout << (std::string)buffer.GetString() << std::endl;

	//std::cout << r.size() << std::endl;
	std::cout << "INFO : indexed " << std::endl;

	delete wordIterator;
}


