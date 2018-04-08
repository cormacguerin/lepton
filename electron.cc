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

std::string getMorphCandidate(string a, string b) {

	//std::wstring wlcs = lcs(converter.from_bytes(a), converter.from_bytes(b));
	//std::string lcs = converter.to_bytes(wlcs);
	//cout << " - - - " << endl;
	//cout << "a           : " << a << endl;
	//cout << "b           : " << b << endl;
	std::string lcs_ = converter.to_bytes(lcs(converter.from_bytes(a), converter.from_bytes(b)));
	inflection i;
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
				i.left.push_back(inflect_left_a);
				inflections_left.insert(std::pair<string, int>(inflect_left_a, inflections_left[inflect_left_a]++));
				//			cout << "inflect_left_a " << inflect_left_a << endl;
			}
		}
		if (!inflect_left_b.empty()) {
			if (inflect_left_b.length() <= lcs_.length()) {
				i.left.push_back(inflect_left_b);
				inflections_left.insert(std::pair<string, int>(inflect_left_b, inflections_left[inflect_left_b]++));
				//			cout << "inflect_left_b " << inflect_left_b << endl;
			}
		}
		if (!inflect_right_a.empty()) {
			if (inflect_right_a.length() <= lcs_.length()) {
				i.right.push_back(inflect_right_a);
				inflections_right.insert(std::pair<string, int>(inflect_right_a, inflections_right[inflect_right_a]++));
				//			cout << "inflect_right_a " << inflect_right_a << endl;
			}
		}
		if (!inflect_right_b.empty()) {
			if (inflect_right_b.length() <= lcs_.length()) {
				i.right.push_back(inflect_right_b);
				inflections_right.insert(std::pair<string, int>(inflect_right_b, inflections_right[inflect_right_b]++));
				//			cout << "inflect_right_b " << inflect_right_b << endl;
			}
		}
	}
	inflections.push_back(i);

	return "s";
}

/*
 * WARNING - complicated function.
 *
 * This function sorts the map by the values rather than keys with most common at the end.
 * We then filter the common ones against known unigrams and gather as many as requested.
 * options: 
 * 	map - candidates map of inflections against occurrences.
 *  unigram - unigram vector from sentencepiece.
 *  no_to_gather - number of candidates we should gather.
 */
void gatherInflections(std::map<string, int> map, std::map<string, int> unigrams, int no_to_gather) {
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
}

std::string trim(const std::string& str, const std::string& whitespace = " \t") {
	const auto strBegin = str.find_first_not_of(whitespace);
	if (strBegin == std::string::npos)
		return ""; // no content

	const auto strEnd = str.find_last_not_of(whitespace);
	const auto strRange = strEnd - strBegin + 1;

	return str.substr(strBegin, strRange);
}

// todo: this is slow af, needs to do something about it.
std::string sanitizeText(std::string str) {
	auto isPunct = [](char c) { 
		return std::ispunct(static_cast<unsigned char>(c));
	};

	// remove punctuations
	str.erase(std::remove_if(str.begin(), str.end(), isPunct), str.end());
	// remove digits
	str.erase(std::remove_if(str.begin(), str.end(), isPunct), str.end());

	// convert to lowercase
	std::transform(str.begin(), str.end(), str.begin(), [](char c) {
			return std::tolower(static_cast<unsigned char>(c));
			});
	return str;
}

void process(const char* a, const char* b, const char* c) {
	ifstream vocab(a);
	if (a) {
		string line;
		string last_line = "";
		while (getline(vocab, line)) {
			line.erase(std::find(line.begin(), line.end(), '\t'), line.end());
			line = sanitizeText(line);
			getMorphCandidate(last_line, line);
			last_line = line;
		}
	}
	if (a) {
		string line;
		while (getline(vocab, line)) {
			line.erase(std::find(line.begin(), line.end(), '\t'), line.end());
			line = sanitizeText(line);
			//getUnigrams(line);
		}
	}
	//gatherInflections();
	/*
	   for (std::map<std::string, int>::const_iterator it = roots.begin(); it != roots.end(); ++it) {
	   cout << (*it).first << "     " << (*it).second << endl;
	   }
	// anything with occurrences than say 10 would be pretty pathetic, lets discard.
	for (std::map<std::string, int>::const_iterator it = inflections_right.begin(); it != inflections_right.end(); ++it) {
	cout << (*it).first << "     " << (*it).second << endl;
	}
	for (std::map<std::string, int>::const_iterator it = inflections_left.begin(); it != inflections_left.end(); ++it) {
	cout << (*it).first << "     " << (*it).second << endl;
	}
	*/
}


int main(int argc, char** argv)
{
	seg = L'▁';
	process(argv[1], argv[2], argv[3]);
}

