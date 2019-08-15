#ifndef _TEXTTOOLS_H_
#define _TEXTTOOLS_H_

#include <algorithm>
#include <string>

extern std::string trim(const std::string& str, const std::string& whitespace = " \t") {
	const auto strBegin = str.find_first_not_of(whitespace);
	if (strBegin == std::string::npos)
		return ""; // no content

	const auto strEnd = str.find_last_not_of(whitespace);
	const auto strRange = strEnd - strBegin + 1;

	return str.substr(strBegin, strRange);
}

extern std::string sanitizeText(std::string str) {
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
extern std::string toLowerCase(std::string str) {
	std::transform(str.begin(), str.end(), str.begin(), [](char c) {
			return std::tolower(static_cast<unsigned char>(c));
			});
	return str;
}

/* 
 * If this has special characters it's probably not a good candidate so just remove.
 */
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

#endif
