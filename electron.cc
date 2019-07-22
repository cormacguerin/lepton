#include <iostream>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <set>
#include <map>
#include <vector>
#include <locale>
#include <codecvt>
#include <string>
#include <exception>
#include <regex>
#include <numeric>
#include "texttools.h"


using namespace std;

std::set<std::string> words;
std::map<std::string,int> roots;
std::map<std::string,int> inflections_left;
std::map<std::string,int> inflections_right;
std::map<std::string,float> unigrams;
std::map<std::string,std::vector<string>> synonyms;
std::vector<std::pair<string,int>> licvpao;
std::vector<std::pair<string,int>> ricvpao;

std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

struct inflection {
	std::string root;
	std::vector<std::string> left;
	std::vector<std::string> right;
}; 
std::vector<inflection> inflections;

std::wstring lcs( std::wstring a, std::wstring b ) {
	if( a.empty() || b.empty() ) return {} ;

	wstring current_lcs = L"";
	for(int i=0; i< a.length(); i++) {
		size_t fpos = b.find(a[i], 0);
		while(fpos != wstring::npos) {
			wstring tmp_lcs = L"";
			tmp_lcs += a[i];
			if (tmp_lcs.length() > current_lcs.length()) {
				current_lcs = tmp_lcs;
			}
			for (int x = fpos+1; x < b.length(); x++) {
				tmp_lcs+=b[x];
				size_t spos = a.find(tmp_lcs, 0);
				if (spos != wstring::npos) {
					if (tmp_lcs.length() > current_lcs.length()) {
						current_lcs = tmp_lcs;
					}
				}
			}
			fpos = b.find(a[i], fpos+1);
		}
	}
	if (current_lcs.length() < 2) {
		current_lcs = L"";
	}
	return current_lcs;
}

std::pair<string, float> getUnigrams(string line) {
	int pos = line.find_first_of(' ');
	std::istringstream iss(line);
	std::string unigram = line.substr(0, pos);
	float score = stof(line.substr(pos+1));
	return std::pair<string, float>(unigram, score);
}

/*
 * Given two strings, get the root using lcs algorithm above
 * From the root we can assume 
 *  - the left of the stem is a potential left inflections (eg. kuma -> oguma, in japanese)
 *  - the right of the stem is a potential right side inflection (eg. run -> (ing, er) in english)
 *  We store these in a map, at insertion time we consider the following conditionals.
 *  - that the inflection exitsts (is not empty).
 *  - that the inflection is not a stop word(currently not implemented, might filter elsewhere).
 *  - that the inflection is not longer than the stem (generally that would be weird for an inflection?).
 *  - that the inflection is one of our extracted unigrams from our segmenter (should help filter out bad ones).
 */
void getMorphCandidate(string a, string b) {

	inflection i;
	std::string lcs_ = converter.to_bytes(lcs(converter.from_bytes(a), converter.from_bytes(b)));
	if (!lcs_.empty()) {
		i.root = lcs_;
		roots.insert(std::pair<string, int>(lcs_, roots[lcs_]++));

		//	cout << "lcs         :  " << lcs_ << endl;
		std::string inflect_left_a = a.substr(0, a.find(lcs_));
		std::string inflect_left_b = b.substr(0, b.find(lcs_));
		std::string inflect_right_a = a.substr(a.find(lcs_)+lcs_.length(), a.length());
		std::string inflect_right_b = b.substr(b.find(lcs_)+lcs_.length(), b.length());
		if (!inflect_left_a.empty()) {
			if (inflect_left_a.length() <= lcs_.length()) {
//				map<string, float>::const_iterator it = unigrams.find(inflect_left_a);
//				if (it!=unigrams.end()) {
					i.left.push_back(inflect_left_a);
					inflections_left.insert(std::pair<string, int>(inflect_left_a, inflections_left[inflect_left_a]++));
//				}
			}
		}
		if (!inflect_left_b.empty()) {
			if (inflect_left_b.length() <= lcs_.length()) {
//				map<string, float>::const_iterator it = unigrams.find(inflect_left_b);
//				if (it!=unigrams.end()) {
					i.left.push_back(inflect_left_b);
					inflections_left.insert(std::pair<string, int>(inflect_left_b, inflections_left[inflect_left_b]++));
//				}
			}
		}
		if (!inflect_right_a.empty()) {
			if (inflect_right_a.length() <= lcs_.length()) {
//				map<string, float>::const_iterator it = unigrams.find(inflect_right_a);
//				if (it!=unigrams.end()) {
					i.right.push_back(inflect_right_a);
					inflections_right.insert(std::pair<string, int>(inflect_right_a, inflections_right[inflect_right_a]++));
//				}
			}
		}
		if (!inflect_right_b.empty()) {
			if (inflect_right_b.length() <= lcs_.length()) {
//				map<string, float>::const_iterator it = unigrams.find(inflect_right_b);
//				if (it!=unigrams.end()) {
					i.right.push_back(inflect_right_b);
					inflections_right.insert(std::pair<string, int>(inflect_right_b, inflections_right[inflect_right_b]++));
//				}
			}
		}
	}
	inflections.push_back(i);
}

/*
 * This function sorts the map by the values rather than keys with most common at the end.
 * We then filter the common ones against known unigrams and gather as many as requested.
 * return a vector or pairs.
 * Only process potential inflections for 10 or more mathes. This should be configurable.
 */
std::vector<std::pair<string,int>> mapToSortedVectorPair(std::map<string, int> map) {
	std::vector<std::pair<string, int>> mapVector;
	for (auto iterator = map.begin(); iterator != map.end(); ++iterator) {
		if (std::get<1>(*iterator) > 99) {
			mapVector.push_back(*iterator);
		}
	}
	std::sort(mapVector.begin(), mapVector.end(),
			[](const std::pair<string, int> &x,
				const std::pair<string, int> &y)
			{
			return x.second < y.second;
			});
	for (std::vector<std::pair<string,int>>::const_iterator it = mapVector.begin() ; it != mapVector.end(); it++){
		cout << (*it).first << " " << (*it).second << endl;
	}
	return mapVector;
}

/*
 * WARNING - complicated function.
 *
 * 	licvpao - left inflection candidates vector pair against occurrences.
 * 	ricvpao - left inflection candidates vector pair against occurrences.
 *  uvmpas - unigram vector map pair against score.
 *  noctg - number of candidates to gather.
 */
void gatherInflections() {
	licvpao = mapToSortedVectorPair(inflections_left);
	ricvpao = mapToSortedVectorPair(inflections_right);
}

/*
 * There are two filters for generating synonyms.
 * 1. The root must be longer than the inflection.
 * 2. The synonym must be in the corpus of words.
 */
void buildSynonyms() {
	for (std::map<std::string, int>::const_iterator rit = roots.begin(); rit != roots.end(); ++rit) {
		// cout << "stem : " << (*rit).first << " - "<< (*rit).second << endl;
		std::vector<string> syns;
		for (std::vector<std::pair<string, int>>::const_iterator ilit = licvpao.begin(); ilit != licvpao.end(); ++ilit) {
			std::string candidate = (*ilit).first + (*rit).first;
			if ((*rit).first.length() > (*ilit).first.length()) {
				if(words.find(candidate) != words.end()) {
					syns.push_back(candidate);
				}
			}
		}
		for (std::vector<std::pair<std::string, int>>::const_iterator irit = ricvpao.begin(); irit != ricvpao.end(); ++irit) {
			std::string candidate = (*rit).first + (*irit).first;
			if ((*rit).first.length() > (*irit).first.length()) {
				if (words.find(candidate) != words.end()) {
					syns.push_back(candidate);
				}
			}
		}
		if (!syns.empty()) {
			synonyms.insert(std::pair<string, std::vector<string>>((*rit).first, syns));
		}
	}
}

/*
 * process inputs and build the inflections.
 * 
 * note: dictionary files should be space separated pairs of (word, float)
 *  	 number closer to zero is more important. 	  
 *  	 I'm using sentencepiece to train these files.
 *  	 Typically latin languages will use both word and unigram dictionaries
 *  	 However chinese for example you would just provide the unigram for both.
 *
 * options :
 *
 * 	w - word dictionary file ()
 * 	u - unigram dictionary file ()
 * 	n - number of inflections to gather.
 */
void process(const char* w) {

	ifstream word_dict(w);
	if (w) {

		string line;
		/*
		while (getline(uni_dict, line)) {
			try {
				unigrams.insert(getUnigrams(line));
			} catch (exception& e) {
				cout << "Exception in getUnigrams: " << e.what() << endl;
			}
		}
		*/

		string last_line = "";
		while (getline(word_dict, line)) {
			// extract the word (removing any weight / other componetns after)
			line.erase(std::find(line.begin(), line.end(), ' '), line.end());
			line = toLowerCase(line);
			if (isWord(line)) {
				line = sanitizeText(line);
				words.insert(line);
				getMorphCandidate(last_line, line);
			}
			last_line = line;
		}
	}

	gatherInflections();
	buildSynonyms();
/*
	for (std::map<std::string, int>::const_iterator it = inflections_right.begin(); it != inflections_right.end(); ++it) {
		cout << (*it).first << "     " << (*it).second << endl;
	}
	for (std::map<std::string, int>::const_iterator it = roots.begin(); it != roots.end(); ++it) {
	   cout << (*it).first << "     " << (*it).second << endl;
	}
	for (std::map<std::string, int>::const_iterator it = inflections_right.begin(); it != inflections_right.end(); ++it) {
		cout << (*it).first << "     " << (*it).second << endl;
	}
	for (std::map<std::string, int>::const_iterator it = inflections_left.begin(); it != inflections_left.end(); ++it) {
		cout << (*it).first << "     " << (*it).second << endl;
	}
	for (std::map<std::string, float>::const_iterator it = unigrams.begin(); it != unigrams.end(); ++it) {
		cout << (*it).first << "     " << (*it).second << endl;
	}
	for (std::map<std::string, std::vector<string>>::const_iterator it = synonyms.begin(); it != synonyms.end(); ++it) {
		string syns = accumulate(synonyms.begin(), synonyms.end(), string(", "));
		cout << (*it).first << "     " << syns << endl;
	}
*/
	// print the synonyms to stdout.
	for (std::map<std::string, std::vector<string>>::const_iterator it = synonyms.begin(); it != synonyms.end(); ++it) {
		string syns; 
		for (auto const& s : (*it).second) { syns += " ] [ " + s; }
		syns.erase(0, 3);
		syns += " ]";
		cout << (*it).first << "     " << syns << endl;
	}
}


int main(int argc, char** argv)
{
	process(argv[1]);
}

