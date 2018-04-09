#include <iostream>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <map>
#include <vector>
#include <locale>
#include <codecvt>
#include <string>
#include <exception>


using namespace std;

std::map<std::string,int> roots;
std::map<std::string,int> inflections_left;
std::map<std::string,int> inflections_right;
std::map<std::string,float> unigrams;

std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;

wstring seg;

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
	if (current_lcs == seg ||
			current_lcs.length() < 2) {
		current_lcs = L"";
	}
	return current_lcs;
}

std::pair<string, float> getUnigrams(string line) {
	int pos = line.find_first_of('\t');
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
				map<string, float>::const_iterator it = unigrams.find(inflect_left_a);
				if (it!=unigrams.end()) {
					i.left.push_back(inflect_left_a);
					inflections_left.insert(std::pair<string, int>(inflect_left_a, inflections_left[inflect_left_a]++));
				}
			}
		}
		if (!inflect_left_b.empty()) {
			if (inflect_left_b.length() <= lcs_.length()) {
				map<string, float>::const_iterator it = unigrams.find(inflect_left_b);
				if (it!=unigrams.end()) {
					i.left.push_back(inflect_left_b);
					inflections_left.insert(std::pair<string, int>(inflect_left_b, inflections_left[inflect_left_b]++));
				}
			}
		}
		if (!inflect_right_a.empty()) {
			if (inflect_right_a.length() <= lcs_.length()) {
				map<string, float>::const_iterator it = unigrams.find(inflect_right_a);
				if (it!=unigrams.end()) {
					i.right.push_back(inflect_right_a);
					inflections_right.insert(std::pair<string, int>(inflect_right_a, inflections_right[inflect_right_a]++));
				}
			}
		}
		if (!inflect_right_b.empty()) {
			if (inflect_right_b.length() <= lcs_.length()) {
				map<string, float>::const_iterator it = unigrams.find(inflect_right_b);
				if (it!=unigrams.end()) {
					i.right.push_back(inflect_right_b);
					inflections_right.insert(std::pair<string, int>(inflect_right_b, inflections_right[inflect_right_b]++));
				}
			}
		}
	}
	inflections.push_back(i);
}

/*
 * This function sorts the map by the values rather than keys with most common at the end.
 * We then filter the common ones against known unigrams and gather as many as requested.
 * return a vector or pairs.
 */
std::vector<std::pair<string,int>> mapToSortedVectorPair(std::map<string, int> map) {
	std::vector<std::pair<string, int>> mapVector;
	for (auto iterator = map.begin(); iterator != map.end(); ++iterator) {
		mapVector.push_back(*iterator);
	}
	std::sort(mapVector.begin(), mapVector.end(),
			[](const std::pair<string, int> &x,
				const std::pair<string, int> &y)
			{
			return x.second < y.second;
			});
	for (std::vector<std::pair<string,int>>::const_iterator it = mapVector.begin() ; it != mapVector.end(); it++){
		//cout << (*it).first << " " << (*it).second << endl;
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
	std::vector<std::pair<string,int>> licvpao = mapToSortedVectorPair(inflections_left);
	std::vector<std::pair<string,int>> ricvpao = mapToSortedVectorPair(inflections_right);
}

std::string trim(const std::string& str, const std::string& whitespace = " \t") {
	const auto strBegin = str.find_first_not_of(whitespace);
	if (strBegin == std::string::npos)
		return ""; // no content

	const auto strEnd = str.find_last_not_of(whitespace);
	const auto strRange = strEnd - strBegin + 1;

	return str.substr(strBegin, strRange);
}

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
}

// convert to lowercase
std::string toLowerCase(std::string str) {
	std::transform(str.begin(), str.end(), str.begin(), [](char c) {
			return std::tolower(static_cast<unsigned char>(c));
			});
	return str;
}

// todo: this is slow af, needs to do something about it.
bool isWord(std::string str) {
	for (int i=0; i < str.size(); i++) {
		if (isdigit(str.at(i))) {
			return false;
		}
		if (ispunct(str.at(i))) {
			return false;
		}
	}
	return true;
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
void process(const char* w, const char* u, const char* n) {
	ifstream uni_dict(u);
	if (u) {
		string line;
		while (getline(uni_dict, line)) {
			try {
				unigrams.insert(getUnigrams(line));
			} catch (exception& e) {
				cout << "Exception in getUnigrams: " << e.what() << endl;
			}
		}
	}

	ifstream word_dict(w);
	if (w) {
		string line;
		string last_line = "";
		while (getline(word_dict, line)) {
			// extract the word (removing any weight / other componetns after)
			line.erase(std::find(line.begin(), line.end(), '\t'), line.end());
			line = toLowerCase(line);
			if (isWord(line)) {
				//line = sanitizeText(line);
				getMorphCandidate(last_line, line);
			}
			last_line = line;
		}
	}

	gatherInflections();
/*
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
*/
	for (std::map<std::string, int>::const_iterator it = inflections_right.begin(); it != inflections_right.end(); ++it) {
		cout << (*it).first << "     " << (*it).second << endl;
	}
}


int main(int argc, char** argv)
{
	seg = L'▁';
	process(argv[1], argv[2], argv[3]);
}

