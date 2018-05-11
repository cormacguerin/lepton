#include <unicode/ures.h>
#include <unicode/unistr.h>
#include <unicode/resbund.h>
#include <unicode/ustdio.h>
#include <unicode/putil.h>
#include <unicode/msgfmt.h>
#include <unicode/calendar.h>
#include <unicode/brkiter.h>
#include <unicode/uniset.h>
#include <exception>
#include <streambuf>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>

#define N_GRAM_SIZE 3

std::string str_in;
std::map<std::string,std::vector<int>> gramPositions;
std::vector<UnicodeString> grams;
std::vector<std::string> uni_spec;
std::vector<std::string> ja_stop_words;
std::vector<std::string> en_stop_words;

int main() {

  std::ifstream input("input.txt");

  std::string str_in((std::istreambuf_iterator<char>(input)),
    std::istreambuf_iterator<char>());

  UnicodeString uni_str = str_in.c_str();

  std::ifstream uni_spec_dict("data/unicode_special_chars.txt");
  std::ifstream ja_stop_words_dict("data/japanese_stop_words.txt");
  std::ifstream en_stop_words_dict("data/english_stop_chars.txt");

  std::string line;

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

  UErrorCode status = U_ZERO_ERROR;
  BreakIterator *wordIterator = BreakIterator::createWordInstance(Locale("ja","JAPAN"), status);
  std::string str = "Unicode provides a unique number for every character, no matter what the platform, no matter what the program, no matter what the language.";
  wordIterator->setText(uni_str);
  int32_t p = wordIterator->first();
  int32_t l = p;
  int i = 0;
  while (p != BreakIterator::DONE) {
    // printf("Boundary at position %d\n", p);
    p = wordIterator->next();
    std::string converted;
    UnicodeString tmp = uni_str.tempSubString(l,p-l);
    tmp.toUTF8String(converted);
    grams.push_back(tmp);
    l=p;
    // skip stopwords.
    if ( std::find(uni_spec.begin(), uni_spec.end(), converted) != uni_spec.end() ) {
      i++;
      continue;
    }
    if ( std::find(ja_stop_words.begin(), ja_stop_words.end(), converted) != ja_stop_words.end() ) {
      i++;
      continue;
    }
    if ( std::find(en_stop_words.begin(), en_stop_words.end(), converted) != en_stop_words.end() ) {
      i++;
      continue;
    }
    // insert the vector occurrence position.
    gramPositions[converted].push_back(i);
    i++;
  }

  std::map<std::string,int> nGrams;
  for (std::map<std::string, std::vector<int>>::iterator it = gramPositions.begin(); it != gramPositions.end(); it++ ) {
    // do not process for single occurrences.
    if ((it->second).size() > 1) {
      for (std::vector<int>::iterator pit = it->second.begin(); pit != it->second.end(); pit++ ) {
        int n = 1;
        UnicodeString tmp = grams.at(*pit);
        while (n < N_GRAM_SIZE) {
          if (*pit < grams.size() - N_GRAM_SIZE) {
            std::string converted;
            std::string converted_gram;
            UnicodeString this_gram = grams.at(*pit+n);
            this_gram.toUTF8String(converted_gram);
            tmp += grams.at(*pit+n);
            tmp.toUTF8String(converted);
            // skip ngrams ending in a stopword.
            if ( std::find(ja_stop_words.begin(), ja_stop_words.end(), converted_gram) != ja_stop_words.end() ) {
              std::cout << "skip " << converted_gram << std::endl;
            } else {
              nGrams[converted]++;
            }
          }
          n++;
        }
      }
    }
  }

  // At this point we have a map of nGram candidates and a map of words/terms/unigrams
  // We perform some reduction on the nGrams (we only want the largest unique
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
  // But thinks starting or ending with stopwords we remove.
  // eg. the smashing pumpkins = smashing punkins.
  //     the dark tower = dark tower
  //     top of the morning to you = top of the morning
  // 
  for (std::map<std::string, int>::iterator git = nGrams.begin(); git != nGrams.end(); git++ ) {
    if (git->second > 1) {
      if (std::next(git) != nGrams.end()) {
   //   std::cout << " - - - - " << std::endl;
   //   std::cout << "current " << git->first << " " << git->second << std::endl;
   //   std::cout << "next " << std::next(git)->first << " " << std::next(git)->second << std::endl;
        int nextlen = (std::next(git)->first).length();
        int currentlen = (git->first).length();
        // if the next match is longer maybe we should use that instead.
        if (nextlen >= currentlen) {
          if ((std::next(git)->first).substr(0,currentlen) == git->first) {
            if ((std::next(git)->second != git->second)) {
              // the next one has less matches so this one is still valid. Add!
              std::cout << "add " << git->first << " " << git->second << std::endl;
            } else {
              // the next one is a longer maching candidate so skip this one.
              continue;
              std::cout << "skip " << (git->first).substr(0,nextlen) << git->second << std::endl;
            }
          } else {
            // the next one is not matching meaning this is the last match. Add!
            std::cout << "add (not matching) " << git->first << " " << git->second << std::endl;
          }
        } else {
          // the next one has a greater length
          std::cout << "add (next is less) " << git->first << " " << git->second << std::endl;
        }
      }
    }
  }


  delete wordIterator;
  return 0;
}

