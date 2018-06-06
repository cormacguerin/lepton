#include "segmenter.h"
#include <iostream>

Segmenter::Segmenter()
{
}

Segmenter::~Segmenter()
{
}

void Segmenter::init() {
	//sentencepiece::Segmenter processor;
	processor.LoadOrDie("word.model");
	//spec = processor.model_proto().normalizer_spec();
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



std::string Segmenter::tokenize() {

	// this is a redis connection (were replacing this with postgres for the index)
	//	client.connect();
	// postgres connection
	pqxx::connection* C;
	try {
		C = new pqxx::connection("dbname = index user = postgres password = FSa7+aE1vztVIUZiwAt03d4O7YO2Acm6YVyrGloDZKk= hostaddr = 127.0.0.1 port = 5432");
      if (C->is_open()) {
		  std::cout << "Opened database successfully: " << C->dbname() << std::endl;
      } else {
		  std::cout << "Can't open database" << std::endl;
      }
	} catch (const std::exception &e) {
		std::cerr << e.what() << std::endl;
	}

	std::ifstream input("input.txt");

	std::string str_in((std::istreambuf_iterator<char>(input)),
			std::istreambuf_iterator<char>());

	// convert to lowercase
	std::transform((str_in).begin(), (str_in).end(), (str_in).begin(), ::tolower);

	UnicodeString uni_str = str_in.c_str();

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

	UErrorCode status = U_ZERO_ERROR;
	// BreakIterator *wordIterator = BreakIterator::createWordInstance(Locale("ja","JAPAN"), status);
	BreakIterator *wordIterator = BreakIterator::createWordInstance(Locale("en","US"), status);
	wordIterator->setText(uni_str);
	int32_t p = wordIterator->first();
	int32_t l = p;
	int i = 0;

	int ja_stop_words_count = 0;
	int en_stop_words_count = 0;

	while (p != BreakIterator::DONE) {
		// printf("Boundary at position %d\n", p);
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
		if ( std::find(ja_stop_words.begin(), ja_stop_words.end(), converted) != ja_stop_words.end() ) {
			ja_stop_words_count++;
		} else if ( std::find(en_stop_words.begin(), en_stop_words.end(), converted) != en_stop_words.end() ) {
			en_stop_words_count++;
		}
		// insert the vector occurrence position.
		trim(converted);
		if (converted.empty()) {
			continue;
		}
		UnicodeString uc = UnicodeString::fromUTF8(converted);
		grams.push_back(uc);
		gramPositions[converted].push_back(i);
		i++;
	}

	std::cout << "en stop cont " << en_stop_words_count << std::endl;
	std::cout << "ja stop cont " << ja_stop_words_count << std::endl;
	std::map<std::string,int> nGrams;
	for (std::map<std::string, std::vector<int>>::iterator it = gramPositions.begin(); it != gramPositions.end(); it++ ) {
		// do not process for single occurrences.
		if ((it->second).size() > 1) {
			for (std::vector<int>::iterator pit = it->second.begin(); pit != it->second.end(); pit++ ) {
				int n = 0;
				UnicodeString tmp;
				while (n < N_GRAM_SIZE) {
					if (*pit < grams.size() - N_GRAM_SIZE) {
						std::string converted;
						std::string converted_gram;
						UnicodeString this_gram = grams.at(*pit+n);
						this_gram.toUTF8String(converted_gram);
						// skip ngrams ending in a stopword
						tmp += grams.at(*pit+n);
						if (IS_CJK == false) {
							tmp += " ";
						}
						/*
						for(std::vector<std::string>::iterator it = en_stop_words.begin(); it != en_stop_words.end(); ++it) {
							std::cout << " - " << n << " - " << *it << " : " << converted_gram << std::endl;
						}
						*/
						if ( std::find(ja_stop_words.begin(), ja_stop_words.end(), converted_gram) != ja_stop_words.end() ) {
							if (n == 0) {
								break;
							}
							n++;
							continue;
						}
						if ( std::find(en_stop_words.begin(), en_stop_words.end(), converted_gram) != en_stop_words.end() ) {
							if (n == 0) {
								break;
							}
							n++;
							continue;
						}
						if ( std::find(ascii_spec.begin(), ascii_spec.end(), converted_gram) != ascii_spec.end() ) {
							break;
						}
						if (converted_gram.empty()) {
							n++;
							continue;
						} else {
							tmp.toUTF8String(converted);
							trim(converted);
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
	// But things starting or ending with stopwords we remove.
	// eg. the smashing pumpkins = smashing punkins.
	//     the dark tower = dark tower
	//     top of the morning to you = top of the morning
	// 
	rapidjson::Document docngrams;
	docngrams.Parse("{}");
	rapidjson::Document::AllocatorType& allocator = docngrams.GetAllocator();

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
							// std::cout << git->first << " " << git->second << std::endl;
							rapidjson::Value k((git->first).c_str(), allocator);
							docngrams.AddMember(k, rapidjson::Value(git->second), allocator);
						} else {
							// the next one is a longer maching candidate so skip this one.
							continue;
						}
					} else {
						// the next one is not matching meaning this is the last match. Add!
						// std::cout << git->first << " " << git->second << std::endl;
						rapidjson::Value k((git->first).c_str(), allocator);
						docngrams.AddMember(k, rapidjson::Value(git->second), allocator);
					}
				} else {
					// the next one has a greater length
					// std::cout << git->first << " " << git->second << std::endl;
					rapidjson::Value k((git->first).c_str(), allocator);
					docngrams.AddMember(k, rapidjson::Value(git->second), allocator);
				}
			}
		}
	}
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	docngrams.Accept(writer);

	return buffer.GetString();
	/*
	
	C->prepare("insert_to_docs", "insert into docs (url, segmented_grams) values ($1, $2)");
	pqxx::work txn(*C);
	pqxx::result r = txn.prepared("insert_to_docs")("test")(buffer.GetString()).exec();
	txn.commit();
	std::cout << r.size() << std::endl;

	delete wordIterator;

	return 0;
	*/
}


void Segmenter::tokenize(std::string text, std::vector<std::string> *pieces) {
	//processor.Encode(normalizer.Normalize(text), &pieces);
	processor.Encode(text, pieces);
	/*
	for(std::vector<std::string>::iterator it = pieces.begin(); it != pieces.end(); ++it) {
		std::cout << " parsed_contents  b " << *it << std::endl;
	}
	*/
}

void Segmenter::detokenize(std::vector<std::string> pieces, std::string text) {
	processor.Decode(pieces, &text);
//	for (const std::string &token : pieces) {
//		std::cout << token << std::endl;
//	}
}
