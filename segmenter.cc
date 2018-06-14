#include "segmenter.h"
#include "sentence_piece_processor.h"
#include "texttools.h"
#include <chrono>

Segmenter::Segmenter()
{
#define N_GRAM_SIZE 3
#define IS_CJK false
}

Segmenter::~Segmenter()
{
}

void Segmenter::init() {
	
	//sentencepiece::Segmenter processor;
	//spec = processor.model_proto().normalizer_spec();

	std::ifstream ascii_spec_dict("data/ascii_special_characters.txt");
	std::ifstream uni_spec_dict("data/unicode_special_chars.txt");
	std::ifstream ja_stop_words_dict("data/japanese_stop_words.txt");
	std::ifstream en_stop_words_dict("data/english_stop_words.txt");

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

}


void Segmenter::parse(std::string url, std::string lang, std::string str_in) {
	// postgres worker
	pqxx::work txn(*C);

	// temp containsers we use for processing
	std::map<std::string,std::vector<int>> gramPositions;
	std::map<std::vector<std::string>, int> gramCandidates;
	std::vector<std::string> gramWindow;
	std::vector<UnicodeString> grams;

	// this is a redis connection (were replacing this with postgres for the index)
	//	client.connect();
	// postgres connection
	std::cout << "INFO : Start parsing for " << url << std::endl;

	/*
	std::ifstream input("input.txt");

	std::string str_in((std::istreambuf_iterator<char>(input)),
			std::istreambuf_iterator<char>());
	*/

	// convert to lowercase
	std::transform((str_in).begin(), (str_in).end(), (str_in).begin(), ::tolower);

	UnicodeString uni_str = str_in.c_str();

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
		if ( std::find(ja_stop_words.begin(), ja_stop_words.end(), converted) != ja_stop_words.end() ) {
			isStopWord = true;
			ja_stop_words_count++;
		} else if ( std::find(en_stop_words.begin(), en_stop_words.end(), converted) != en_stop_words.end() ) {
			isStopWord = true;
			en_stop_words_count++;
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
		// This solution seems to be slower in my benchmarking...
		// I really thought it would be faster but actually looping
		// the positions as further down seems to be a better algorithm.
		/*
		gramWindow.push_back(converted);
		if (gramWindow.size() > N_GRAM_SIZE) {
			gramWindow.erase(gramWindow.begin());
		}
		std::vector<std::string> tmpgram;
		for (std::vector<std::string>::iterator it = gramWindow.begin(); it != gramWindow.end(); it++) {
			tmpgram.push_back(*it);
			gramCandidates[tmpgram]++;
		}
		*/
		i++;
	}
	std::cout << "INFO : no. grams found " << gramPositions.size() << std::endl;

	std::cout << "en stop cont " << en_stop_words_count << std::endl;
	std::cout << "ja stop cont " << ja_stop_words_count << std::endl;
	// this loop is very slow, I wonder if we just created a new map for all ngram candidates..
	
	for (std::map<std::string, std::vector<int>>::iterator it = gramPositions.begin(); it != gramPositions.end(); it++) {
		// do not process for single occurrences.
		if ((it->second).size() > 1) {
			for (std::vector<int>::iterator pit = it->second.begin(); pit != it->second.end(); pit++ ) {
				std::vector<std::string> tmpgram;
				std::string converted_gram;
				UnicodeString this_gram = grams.at(*pit);
				this_gram.toUTF8String(converted_gram);
				int n = 0;
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
							// skip ngrams ending in a stopword
							if ( std::find(ja_stop_words.begin(), ja_stop_words.end(), converted_gram) != ja_stop_words.end() ) {
								n++;
								continue;
							} else if ( std::find(en_stop_words.begin(), en_stop_words.end(), converted_gram) != en_stop_words.end() ) {
								n++;
								continue;
							} else if ( std::find(ascii_spec.begin(), ascii_spec.end(), converted_gram) != ascii_spec.end() ) {
								n++;
								continue;
							}
							tmp.toUTF8String(converted);
							trimInPlace(converted);
							tmpgram.push_back(converted);
							gramCandidates[tmpgram]++;
						}
					}
					n++;
				}
			}
		}
	}
	
	std::cout << "INFO : no. ngrams found " << gramCandidates.size() << std::endl;

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
							txn.exec(update_grams_table(url, txn.quote(trim(gram).c_str()), git->second));
						} else {
							// the next one is a longer maching candidate so skip this one.
							continue;
						}
					} else {
						rapidjson::Value k((trim(gram).c_str()), allocator);
						docngrams.AddMember(k, rapidjson::Value(git->second), allocator);
						txn.exec(update_grams_table(url, txn.quote(trim(gram)).c_str(), git->second));
					}
				} else {
					rapidjson::Value k((trim(gram).c_str()), allocator);
					docngrams.AddMember(k, rapidjson::Value(git->second), allocator);
					txn.exec(update_grams_table(url, txn.quote(trim(gram).c_str()), git->second));
				}
			}
		}
	}
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	docngrams.Accept(writer);

	//pqxx::work txn(*C);

	std::string update = "UPDATE docs SET (index_date, segmented_grams) = (NOW(), "
		+ txn.quote((std::string)buffer.GetString())
		+ ") WHERE url='"
		+ url
		+ "';";
	txn.exec(update);

	txn.commit();
	//std::cout << r.size() << std::endl;
	std::cout << "INFO : indexed " << std::endl;

	delete wordIterator;
}

std::string Segmenter::update_grams_table(std::string url, std::string gram, int c) {

	std::string update_grams = "INSERT INTO grams (url_id, gram, count) VALUES (" 
		" (SELECT id FROM docs WHERE url = '" + url + "')," + gram + "," + std::to_string(c) + ")"
		+ " ON CONFLICT ON CONSTRAINT grams_pkey DO UPDATE SET count = " + std::to_string(c)
		+ " WHERE grams.url_id = (SELECT id FROM docs WHERE url = '" + url + "') "
		+ " AND 'gram' = " + gram
		+ " ;";
	return update_grams;
}

void Segmenter::tokenize(std::string text, std::vector<std::string> *pieces) {
}

void Segmenter::detokenize(std::vector<std::string> pieces, std::string text) {
}

