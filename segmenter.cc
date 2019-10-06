#include "segmenter.h"
//#include "sentence_piece_processor.h"
#include <chrono>
#include "texttools.h"
#include <future>
#include <math.h>

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
		C = new pqxx::connection("dbname = index user = postgres password = kPwFWfYAsyRGZ6IomXLCypWqbmyAbK+gnKIW437QLjw= hostaddr = 127.0.0.1 port = 5432");
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


void Segmenter::parse(std::string id, std::string url, std::string lang, std::string str_in,
				   std::map<std::string, Frag::Item> &doc_unigram_map,
				   std::map<std::string, Frag::Item> &doc_bigram_map,
				   std::map<std::string, Frag::Item> &doc_trigram_map) {
	// postgres worker
	pqxx::work txn(*C);

	// temp containsers we use for processing
	std::map<std::string,std::vector<int>> gramPositions;
	std::map<std::vector<std::string>, int> gramCandidates;
	std::vector<std::string> gramWindow;
	std::vector<icu::UnicodeString> grams;

	// this is a redis connection (were replacing this with postgres for the index)
	// client.connect();
	// postgres connection
	std::cout << "INFO : Start parsing for " << url << std::endl;

	/*
	std::ifstream input("input.txt");

	std::string str_in((std::istreambuf_iterator<char>(input)),
			std::istreambuf_iterator<char>());
	*/

	// convert to lowercase
	std::transform((str_in).begin(), (str_in).end(), (str_in).begin(), ::tolower);

	icu::UnicodeString uni_str = str_in.c_str();

	UErrorCode status = U_ZERO_ERROR;
	// BreakIterator *wordIterator = BreakIterator::createWordInstance(Locale("ja","JAPAN"), status);
	icu::BreakIterator *wordIterator = icu::BreakIterator::createWordInstance(icu::Locale("en","US"), status);
	wordIterator->setText(uni_str);
	int32_t p = wordIterator->first();
	int32_t l = p;

	int ja_stop_words_count = 0;
	int en_stop_words_count = 0;

	std::vector<std::string> gramholder[N_GRAM_SIZE];
	std::vector<bool> stopholder[N_GRAM_SIZE];

	// for simplicity were going to just count every term (for caculating term frequency)
	int gramcount=0;

	while (p != icu::BreakIterator::DONE) {

		gramcount++;
		bool isStopWord = false;
		p = wordIterator->next();
		std::string converted;
		icu::UnicodeString tmp = uni_str.tempSubString(l,p-l);
		tmp.toUTF8String(converted);
		l=p;
		
		// skip special characters (we should perhaps strip all this out before getting into the segmenter)
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
		
//		UnicodeString uc = UnicodeString::fromUTF8(converted);
//		grams.push_back(uc);

		if ( std::find(ja_stop_words.begin(), ja_stop_words.end(), converted) != ja_stop_words.end() ) {
			isStopWord = true;
		} 
		if ( std::find(en_stop_words.begin(), en_stop_words.end(), converted) != en_stop_words.end() ) {
			isStopWord = true;
		}
		// prints the stream of tile strings.
		// std::cout << converted << std::endl;

		for (int j=0; j < N_GRAM_SIZE; j++) {
			gramholder[j].push_back(converted);
			stopholder[j].push_back(isStopWord);
			if (gramholder[j].size() > N_GRAM_SIZE-j) {
				gramholder[j].erase(gramholder[j].begin());
				stopholder[j].erase(stopholder[j].begin());
			}
			if (stopholder[j].back() == false && stopholder[j].at(0) == false) {
				gramCandidates[gramholder[j]]++;
			}
		}
	}
	
	delete wordIterator;

	//std::cout << "INFO : no. grams found " << gramPositions.size() << std::endl;
	// I thought it would be better to do a double pass like this but it's about twice as slow unfortunately.
	// The brute force above is much faster.. I think the only real way to make this much faster is not to use stl..

	/*
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
	*/
	
	// At this point we have a map of nGram candidates and a map of words/terms/unigrams
	// We perform some reduction on the gramCandidates (we only want the largest unique
	// matches)
	//
	// NEXT 6 LINES ARE WROONG> IT will break query matching
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

	/*
	prepare_insert_unigram(*C, lang);
	prepare_insert_bigram(*C, lang);
	prepare_insert_trigram(*C, lang);
	*/
	// prepare_known_insert(*C);
	//
	
	std::vector<int> term_incidence;
	for (std::map<std::vector<std::string>, int>::iterator git = gramCandidates.begin(); git != gramCandidates.end(); git++ ) {
		// only include grams where there is at least one occurrence.
		// If you include all you get a balooned index.
		// a second entity extraction pass should pick up anything missing.
//		if (git->second > 1) {
			if (std::next(git) != gramCandidates.end()) {
				// std::cout << " - - - - " << std::endl;
				// std::cout << "current " << git->first << " " << git->second << std::endl;
				// std::cout << "next " << std::next(git)->first << " " << std::next(git)->second << std::endl;
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
				rapidjson::Value k((trim(gram).c_str()), allocator);
				docngrams.AddMember(k, rapidjson::Value(git->second), allocator);
				// Very large grams are relatively meaningless. The current database limit is 1024, but even that is big.
				// I notices a lot of bad trigrams. 
				// - For bigrams there need to be two or more occurrences.
				if (trim(gram).size() < 128) {
					// I couldn't find any way to parse the id as a json path is postgres so supplying it directly here instead.
					bool isAdd = false;

					Frag::Item frag_term;
					if ((git->first).size() == 1 && git->second > 0) {
						double tf = (double)git->second/sqrt(gramcount);
						frag_term.url_id = atoi(id.c_str());
						frag_term.weight = 0;
						frag_term.tf = tf;
						doc_unigram_map.insert(std::pair<std::string, Frag::Item>(trim(gram).c_str(),frag_term));
						isAdd = true;
					}
					if ((git->first).size() == 2 && git->second > 2) {
						// unsure about this, should we compensate for fequency with ngrams..
						// in a bi gram for example there are two terms.. so makes sense that there half the number of possibilities..
						double tf = (double)git->second/sqrt((gramcount));
						frag_term.url_id = atoi(id.c_str());
						frag_term.weight = 0;
						frag_term.tf = tf;
						doc_bigram_map.insert(std::pair<std::string, Frag::Item>(trim(gram).c_str(),frag_term));
						isAdd = true;
					}
					// - Same for tri grams we need more occurrences to count them.
					if ((git->first).size() > 2 && git->second > 2) {
						// same as above.
						double tf = (double)git->second/sqrt((gramcount));
						frag_term.url_id = atoi(id.c_str());
						frag_term.weight = 0;
						frag_term.tf = tf;
						doc_trigram_map.insert(std::pair<std::string, Frag::Item>(trim(gram).c_str(),frag_term));
						isAdd = true;
					}
					if (isAdd == false) {
						continue;
					}
					// store incidence of words. The quality calculate is based on language.
					// crawling wikipedia for example show up lots of numbers which are not really relevant in terms of NLP
					// we need to work to reduce the number of odd chars/ints etc.
					bool isWord = true;
					for (auto const& s : git->first) {
						if (s.find_first_of("0123456789") != std::string::npos) {
							isWord=false;
						}
					} 
					if (isWord==true) {
						term_incidence.push_back(git->second);
					}
				} else {
					continue;
				}
				/*
				if (nextlen >= currentlen) {
					if ((std::next(git)->first).at(currentlen-1) == (git->first).back()) {
						if ((std::next(git)->second != git->second)) {
							rapidjson::Value k((trim(gram).c_str()), allocator);
							docngrams.AddMember(k, rapidjson::Value(git->second), allocator);
							r = txn.prepared("insert_grams")(trim(gram).c_str())(id)(std::to_string(git->second)).exec();
	//						r = txn.prepared("insert_known_grams")(100)(id)(std::to_string(git->second)).exec();
						} else {
							// the next one is a longer maching candidate so skip this one.
							continue;
						}
					} else {
						rapidjson::Value k((trim(gram).c_str()), allocator);
						docngrams.AddMember(k, rapidjson::Value(git->second), allocator);
						r = txn.prepared("insert_grams")(trim(gram).c_str())(id)(std::to_string(git->second)).exec();
	//					r = txn.prepared("insert_known_grams")(100)(id)(std::to_string(git->second)).exec();
					}
				} else {
					rapidjson::Value k((trim(gram).c_str()), allocator);
					docngrams.AddMember(k, rapidjson::Value(git->second), allocator);
					r = txn.prepared("insert_grams")(trim(gram).c_str())(id)(std::to_string(git->second)).exec();
	//				r = txn.prepared("insert_known_grams")(100)(id)(std::to_string(git->second)).exec();
				}
				*/
			}
//		}
	}
	// function to measure simple quality.
	// zipfs law stats that the most frequent term should occur twice as many times as the second most frequent,
	// Three times as much as the third and so on. Lets assume zipf's law represents a perfect document.
	// We try to calculate our deviation from zip's law and use this as an error margin. eg.
	// 1 - (deviation from perfect distribution).
	std::sort(term_incidence.rbegin(), term_incidence.rend());
	// hack
	// term_incidence.erase(std::unique(term_incidence.begin(), term_incidence.end()), term_incidence.end());
	double z_deviation = 0.0;
	for (std::vector<int>::iterator it = term_incidence.begin()+1; it != term_incidence.end(); it++ ) {
	//	std::cout << "segmenter.cc : " << it - term_incidence.begin() +1 << " - incidence " << *it << std::endl;
		double r = (double)term_incidence.at(0) / *it;
		double zdev = abs(r-(it-term_incidence.begin()+1))/(it-term_incidence.begin()+1);
	//	std::cout << "segmented.cc r : " << r << " zdev : " << zdev << std::endl;
		z_deviation += abs(r-zdev);
	}
	z_deviation = z_deviation/term_incidence.size();
	// std::cout << "z_deviation : " << z_deviation << std::endl;
	double quality = 1-(1/z_deviation);
	std::cout << "quality : " << quality << std::endl;
	// atf = atf / (doc_unigram_map.size()+doc_bigram_map.size()+doc_trigram_map.size());
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	docngrams.Accept(writer);

	std::string docstable = "docs_" + lang;
	std::string update = "UPDATE " + docstable + " SET (index_date, segmented_grams, quality) = (NOW(), $escape$"
		+ (std::string)buffer.GetString()
		+ "$escape$, " + std::to_string(quality) +") WHERE url='"
		+ url
		+ "';";
	txn.exec(update);

	txn.commit();
	//std::cout << r.size() << std::endl;
	std::cout << "INFO : indexed " << std::endl;

}



/* 
 * prepared CTE function to insert the gram into ngrams table
 * returning the gram id value for updating the docngrams tables 
 * The returning gives about 70% overall performance gain compared with separate insert/select.
 * The prepare gives about 30% overall performance.
 * CTE gives slight performance gain.
 * So basically this improved speed from about 3 docs per second to 6.5 docs per second.
 */
/*
void Segmenter::prepare_insert_unigram(pqxx::connection_base &c, std::string lang) {
	std::string unigramtable = "unigrams_" + lang;
	std::string unigramtable_constraint = "unigrams_" + lang + "_gram_key";
	std::string docunigramtable = "docunigrams_" + lang;
	std::string docunigramtable_constraint = "docunigrams_" + lang + "_pkey";
	c.prepare("insert_unigrams",
		"WITH t as (INSERT INTO " + unigramtable + " (gram) VALUES ($1) "
		"ON CONFLICT ON CONSTRAINT " + unigramtable_constraint + " DO UPDATE SET gram = $1 RETURNING " + unigramtable + ".id) "
		"INSERT INTO " + docunigramtable + " (url_id, gram_id, tf) VALUES ($2, (SELECT id FROM t), $3) "
		"ON CONFLICT ON CONSTRAINT " + docunigramtable_constraint + " DO UPDATE SET tf = $3 "
		"WHERE " + docunigramtable + ".url_id = $2 AND " + docunigramtable + ".gram_id = (SELECT id FROM t)");
}

void Segmenter::prepare_insert_bigram(pqxx::connection_base &c, std::string lang) {
	std::string bigramtable = "bigrams_" + lang;
	std::string bigramtable_constraint = "bigrams_" + lang + "_gram_key";
	std::string docbigramtable = "docbigrams_" + lang;
	std::string docbigramtable_constraint = "docbigrams_" + lang + "_pkey";
	c.prepare("insert_bigrams",
		"WITH t as (INSERT INTO " + bigramtable + " (gram) VALUES ($1) "
		"ON CONFLICT ON CONSTRAINT " + bigramtable_constraint + " DO UPDATE SET gram = $1 RETURNING " + bigramtable + ".id) "
		"INSERT INTO " + docbigramtable + " (url_id, gram_id, tf) VALUES ($2, (SELECT id FROM t), $3) "
		"ON CONFLICT ON CONSTRAINT " + docbigramtable_constraint + " DO UPDATE SET tf = $3 "
		"WHERE " + docbigramtable + ".url_id = $2 AND " + docbigramtable + ".gram_id = (SELECT id FROM t)");
}

void Segmenter::prepare_insert_trigram(pqxx::connection_base &c, std::string lang) {
	std::string trigramtable = "trigrams_" + lang;
	std::string trigramtable_constraint = "trigrams_" + lang + "_gram_key";
	std::string doctrigramtable = "doctrigrams_" + lang;
	std::string doctrigramtable_constraint = "doctrigrams_" + lang + "_pkey";
	c.prepare("insert_trigrams",
		"WITH t as (INSERT INTO " + trigramtable + " (gram) VALUES ($1) "
		"ON CONFLICT ON CONSTRAINT " + trigramtable_constraint + " DO UPDATE SET gram = $1 RETURNING " + trigramtable + ".id) "
		"INSERT INTO " + doctrigramtable + " (url_id, gram_id, tf) VALUES ($2, (SELECT id FROM t), $3) "
		"ON CONFLICT ON CONSTRAINT " + doctrigramtable_constraint + " DO UPDATE SET tf = $3 "
		"WHERE " + doctrigramtable + ".url_id = $2 AND " + doctrigramtable + ".gram_id = (SELECT id FROM t)");
}
*/
void Segmenter::tokenize(std::string text, std::vector<std::string> *pieces) {
}

void Segmenter::detokenize(std::vector<std::string> pieces, std::string text) {
}

