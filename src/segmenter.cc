#include "segmenter.h"
//#include "sentence_piece_processor.h"
#include <chrono>
#include "texttools.h"
#include <future>
#include <math.h>
#include "util.h"

Segmenter::Segmenter()
{
#define N_GRAM_SIZE 5
#define IS_CJK false
}

Segmenter::~Segmenter()
{
  C->disconnect();
  delete C;
}

void Segmenter::init(std::string database) {
	
	//sentencepiece::Segmenter processor;
	//spec = processor.model_proto().normalizer_spec();

	std::ifstream ascii_spec_dict("data/ascii_special_characters.txt");
	std::ifstream uni_spec_dict("data/unicode_special_chars.txt");
	std::ifstream ja_stop_words_dict("data/japanese_stop_words.txt");
	std::ifstream en_stop_words_dict("data/english_stop_words.txt");

    if (!database.empty()) {
        try {
            C = new pqxx::connection("dbname = " + database + " user = postgres password = " + getDbPassword() + " hostaddr = 127.0.0.1 port = 5432");
        if (C->is_open()) {
              std::cout << "Opened database successfully: " << C->dbname() << std::endl;
        } else {
              std::cout << "Can't open database" << std::endl;
            }
        } catch (const std::exception &e) {
            std::cerr << e.what() << std::endl;
        }
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


void Segmenter::parse(std::string id, std::string lang, std::string str_in, std::string table,
				   std::map<std::string, Frag::Item> &doc_unigram_map,
				   std::map<std::string, Frag::Item> &doc_bigram_map,
				   std::map<std::string, Frag::Item> &doc_trigram_map) {
	// postgres worker
	pqxx::work txn(*C);

	// prepared statements
//	C->prepare("delete_doc_text", "UPDATE docs_en SET raw_text = NULL WHERE id=$1");
//	C->prepare("delete_doc_unigrams", "UPDATE docs_en SET unigrams = NULL WHERE id=$1");
//	C->prepare("delete_doc_unigram_p", "UPDATE docs_en SET unigram_positions = NULL WHERE id=$1");
//	C->prepare("update_doc_text", "UPDATE docs_en SET raw_text = array_append(raw_text, $1) WHERE id=$2");
//	C->prepare("update_doc_unigrams", "UPDATE docs_en SET unigrams = array_append(unigrams, $1) WHERE id=$2");
//	C->prepare("update_doc_unigram_p", "UPDATE docs_en SET unigram_positions = array_append(unigram_positions, $1) WHERE id=$2");

	// reset any existing content
//	pqxx::result a_ = txn.prepared("delete_doc_text")(id).exec();
//	pqxx::result b_ = txn.prepared("delete_doc_unigrams")(id).exec();
//	pqxx::result c_ = txn.prepared("delete_doc_unigram_p")(id).exec();

	// temp containsers we use for processing
	std::map<std::string,std::vector<int>> gramPositions;
	std::map<std::vector<std::string>, std::vector<int>> gramCandidates;
	std::vector<std::string> gramWindow;
	std::vector<icu::UnicodeString> grams;

	/*
	std::ifstream input("input.txt");
	std::string str_in((std::istreambuf_iterator<char>(input)),
			std::istreambuf_iterator<char>());
	*/

	// convert to lowercase
	std::transform((str_in).begin(), (str_in).end(), (str_in).begin(), ::tolower);

	icu::UnicodeString uni_str = str_in.c_str();

	UErrorCode status = U_ZERO_ERROR;

    icu::BreakIterator *wordIterator;
	// BreakIterator *wordIterator = BreakIterator::createWordInstance(Locale("ja","JAPAN"), status);
    if (lang == "ja") {
	    wordIterator = icu::BreakIterator::createWordInstance(icu::Locale("ja","JP"), status);
    } else {
	    wordIterator = icu::BreakIterator::createWordInstance(icu::Locale("en","US"), status);
    }
	wordIterator->setText(uni_str);
	int32_t p = wordIterator->first();
	int32_t l = p;

	int ja_stop_words_count = 0;
	int en_stop_words_count = 0;

	std::vector<std::string> gramholder[N_GRAM_SIZE];
	std::vector<bool> stopholder[N_GRAM_SIZE];

	// for simplicity were going to just count every term (for caculating term frequency)
	int gramcount=0;

	rapidjson::Document docngrams;
	docngrams.Parse("{}");
	rapidjson::Document::AllocatorType& allocator = docngrams.GetAllocator();
	rapidjson::Value raw_text(rapidjson::kArrayType);
	rapidjson::Value unigrams(rapidjson::kObjectType);
	rapidjson::Value bigrams(rapidjson::kObjectType);
	rapidjson::Value trigrams(rapidjson::kObjectType);

	while (p != icu::BreakIterator::DONE) {

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
		char specchars[] = "()-,'\"";
		for (unsigned int i = 0; i < strlen(specchars); ++i) {
			converted.erase (std::remove(converted.begin(), converted.end(), specchars[i]), converted.end());
		}

		// insert the vector occurrence position.
		trimInPlace(converted);
		if (converted.empty()) {
			continue;
		} else {
			gramcount++;
		}
		
//		UnicodeString uc = UnicodeString::fromUTF8(converted);
//		grams.push_back(uc);

//		pqxx::result r = txn.prepared("update_doc_text")(trim(converted).c_str())(id).exec();
		rapidjson::Value k((trim(converted).c_str()), allocator);
		raw_text.PushBack(k, allocator);

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
				gramCandidates[gramholder[j]].push_back(gramcount);
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

	std::vector<int> term_incidence;
	for (std::map<std::vector<std::string>, std::vector<int>>::iterator git = gramCandidates.begin(); git != gramCandidates.end(); git++ ) {
		// only include grams where there is at least one occurrence.
		// If you include all you get a balooned index.
		// a second entity extraction pass should pick up anything missing.
//		if (git->second > 1) {
			if (std::next(git) != gramCandidates.end()) {
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
				// Very large grams are relatively meaningless. The current database limit is 1024, but even that is big.
				// I notices a lot of bad trigrams. 
				// - For bigrams there need to be two or more occurrences.
				if (trim(gram).size() < 128) {
					// I couldn't find any way to parse the id as a json path is postgres so supplying it directly here instead.
					bool isAdd = false;

					Frag::Item frag_term;
					if ((git->first).size() == 1 && git->second.size() > 0) {
						double tf = (double)git->second.size()/sqrt(gramcount);
						frag_term.doc_id = atoi(id.c_str());
						frag_term.weight = 0;
						frag_term.tf = tf;
						doc_unigram_map.insert(std::pair<std::string, Frag::Item>(trim(gram).c_str(),frag_term));

						rapidjson::Value k((trim(gram).c_str()), allocator);
						unigrams.AddMember(k, rapidjson::Value(concat_positions(git->second).c_str(), allocator).Move(), allocator);
//						pqxx::result d_ = txn.prepared("update_doc_unigrams")(trim(gram).c_str())(id).exec();
//						pqxx::result e_ = txn.prepared("update_doc_unigram_p")(concat_positions(git->second).c_str())(id).exec();
						isAdd = true;
					}
					if ((git->first).size() == 2 && git->second.size() > 2) {
						// unsure about this, should we compensate for fequency with ngrams..
						// in a bi gram for example there are two terms.. so makes sense that there half the number of possibilities..
						double tf = (double)git->second.size()/sqrt((gramcount));
						frag_term.doc_id = atoi(id.c_str());
						frag_term.weight = 0;
						frag_term.tf = tf;
						doc_bigram_map.insert(std::pair<std::string, Frag::Item>(trim(gram).c_str(),frag_term));

						rapidjson::Value k((trim(gram).c_str()), allocator);
						bigrams.AddMember(k, rapidjson::Value(concat_positions(git->second).c_str(), allocator).Move(), allocator);
						isAdd = true;
					}
					// - Same for tri grams we need more occurrences to count them.
					if ((git->first).size() > 2 && git->second.size() > 2) {
						// same as above.
						double tf = (double)git->second.size()/sqrt((gramcount));
						frag_term.doc_id = atoi(id.c_str());
						frag_term.weight = 0;
						frag_term.tf = tf;
						doc_trigram_map.insert(std::pair<std::string, Frag::Item>(trim(gram).c_str(),frag_term));

						rapidjson::Value k((trim(gram).c_str()), allocator);
						trigrams.AddMember(k, rapidjson::Value(concat_positions(git->second).c_str(), allocator).Move(), allocator);
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
						term_incidence.push_back(git->second.size());
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

	// Function to measure simple quality.
	// I need some kind of quality score, so we can differentiate noisy documents from good text.
	// I'll say good text is prose, but of course we will want this to be customizable depending on the corpus.
	// 
	// I'm looking for a good default.. I figure if we can use term frequencies to determine what represents a 
	// good document, that might be a good start. Say we try to find a good term frequency distribution that matches
	// high quality prose, we could use that as a bases.
	//
	// zipfs law stats that the most frequent term should occur twice as many times as the second most frequent,
	// Three times as much as the third and so on. Lets assume zipf's law represents a perfect document.
	//
	// And then lets not. I did some testing and on wikipedia it deviates A LOT from zipfs law.
	// I thought it might be something with my indexing, so I got the complete works of shakespere
	// Indexed it, and noticed that it also deviates hevily from zipfs law. What gives.
	//
	// I found another guy who did the same thing.. look at his graph
	// https://fermibot.com/analysis-of-shakespeares-work-using-python-3/
	// It's also deviating a lot.
	//
	// My understanding is that zipfs law is supposed to be log2, but it seems to be closer to ln.
	//
	// We try to calculate our deviation from zip's law and use this as an error margin. eg.
	// We don't need to do the whole document, first 100 or 1000 terms or so should be enough.
	// 
	// I've made some additional modifications described below.
	//
	//
	std::sort(term_incidence.rbegin(), term_incidence.rend());
	
	// METHOD 1
	// using standard deviation method with natural log.
	/*
	double z_variance = 0.0;
	for (std::vector<int>::iterator it = term_incidence.begin()+1; it != term_incidence.end(); it++ ) {
		std::cout << "*it " << *it << std::endl;
		double r = (double)term_incidence.at(0) / *it;
		double ln = log(it-term_incidence.begin()+1);

		double zdevsq = pow(abs(r-ln),2);
		z_variance += zdevsq;
		if ((it-term_incidence.begin()+1) > 1000) {
			z_variance = z_variance/1000;
			break;
		}
		if (std::next(it) == term_incidence.end()) {
			z_variance = z_variance/(it-term_incidence.begin()+1);
			break;
		}
	}
	std::cout << "z_variance : " << z_variance << std::endl;
	double quality;
	if (z_variance>0) {
		quality = 1/sqrt(z_variance);
	} else {
		quality = 0;
	}
	*/

	// METHOD 2
	// This seems to be kinda working, at least it's generally giving a higer score to more contextual documents.
	// There are very few 'bad' mistakes and it's actually very similar to the zipf logic/
	/*
	double z_variance = 0.0;
	for (std::vector<int>::iterator it = term_incidence.begin()+1; it != term_incidence.end(); it++ ) {
		double r = (double)term_incidence.at(0) / *it;
		double ln = log(it-term_incidence.begin()+1);
		//double ln = it-term_incidence.begin()+1;

		// for high quality documents it seems that the sqrt of the ratio r is quite close to
		// the natural log.. actually surprisingly consistant.
		double zdevsq = pow(r-ln,2);
		// double zdevsq = pow(r-ln,2);
		z_variance += zdevsq;
		if ((it-term_incidence.begin()+1) > 100) {
			z_variance = z_variance/100;
			break;
		}
		if (std::next(it) == term_incidence.end()) {
			z_variance = z_variance/(it-term_incidence.begin()+1);
			break;
		}
	}
	double tdscore;
	if (z_variance>0) {
		// were not doing standard deviation this time.
		// just normalize to sane values
		tdscore = 1-(1/(sqrt(z_variance)));
	} else {
		tdscore = 0;
	}
	*/
	double tdscore = 0;

	docngrams.AddMember("raw_text", raw_text, allocator);
	docngrams.AddMember("unigrams", unigrams, allocator);
	docngrams.AddMember("bigrams", bigrams, allocator);
	docngrams.AddMember("trigrams", trigrams, allocator);

	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	docngrams.Accept(writer);

	C->prepare("update_segmented_grams", 
    "UPDATE \"" + table + "\" SET (lt_index_date, lt_segmented_grams, lt_tdscore) = (NOW(), $1, $2) WHERE lt_id = $3");

//  reset any existing content
//	pqxx::result a_ = txn.prepared("delete_doc_text")(id).exec();

	pqxx::result e_ = txn.prepared("update_segmented_grams")((std::string)buffer.GetString())(std::to_string(tdscore))(id).exec();

	txn.commit();

}

void Segmenter::tokenize(std::string text, std::vector<std::string> *pieces) {
}

void Segmenter::detokenize(std::vector<std::string> pieces, std::string text) {
}

std::string Segmenter::getSnippet(std::string text, std::string lang, int position) {

	icu::UnicodeString uni_str = text.c_str();

	UErrorCode status = U_ZERO_ERROR;
    icu::BreakIterator *wordIterator;

    // in this reparse, each space is considered a char, so if you want to match the segmented positions, you need to double.
	// BreakIterator *wordIterator = BreakIterator::createWordInstance(Locale("ja","JAPAN"), status);
    if (lang == "ja") {
	    wordIterator = icu::BreakIterator::createWordInstance(icu::Locale("ja","JP"), status);
    } else {
	    wordIterator = icu::BreakIterator::createWordInstance(icu::Locale("en","US"), status);
    }
	wordIterator->setText(uni_str);
	int32_t p = wordIterator->first();
	int32_t l = p;

    bool start;
    if (position == 0) {
      start = true;
    } else {
      start = false;
    }

	// for simplicity were going to just count every term (for caculating term frequency)
	int gramcount=0;
    int end_position=position+50;
    std::string snippet = "";

	while (p != icu::BreakIterator::DONE) {
		bool isStopWord = false;
        bool skipgram = false;
		p = wordIterator->next();
		std::string converted;
		icu::UnicodeString tmp = uni_str.tempSubString(l,p-l);
		tmp.toUTF8String(converted);
		l=p;
		
		// skip special characters (we should perhaps strip all this out before getting into the segmenter)
		if ( std::find(ascii_spec.begin(), ascii_spec.end(), converted) != ascii_spec.end() ) {
            // continue;
		    skipgram = true;
		}
		if ( std::find(uni_spec.begin(), uni_spec.end(), converted) != uni_spec.end() ) {
            // continue;
		    skipgram = true;
		}
        /*
		char specchars[] = "()-,'\"";
		for (unsigned int i = 0; i < strlen(specchars); ++i) {
			converted.erase (std::remove(converted.begin(), converted.end(), specchars[i]), converted.end());
		}
        */

		// insert the vector occurrence position.
		trimInPlace(converted);
		if (converted.empty()) {
            //continue;
		    skipgram = true;
		}  else {
			// gramcount++;
        }
        if (skipgram == false) {
			gramcount++;
		}
        // std::cout << converted << " " << gramcount << std::endl;
		// skip special characters (we should perhaps strip all this out before getting into the segmenter)
        if (gramcount >= position-25 && start == false) {
            if (skipgram = true) {
                start = true;
            }
            if (gramcount >= position-5) {
                start = true;
            }
        }
        if (start == true) {
            if (converted != " ") {
                snippet += converted;
                // should be for cjk not just japanese
                if (lang == "ja") {
                } else {
                    snippet += " ";
                }
            }
            if (gramcount < position) {
                if (lang == "ja") {
                } else {
                    if (converted == ".") {
                        snippet = "";
                    }
                }
            }
        }
        if (gramcount >= end_position) {
            break;
        }
    }
	delete wordIterator;
    return snippet;
}

std::string Segmenter::concat_positions(std::vector<int> pos) {
	int x = 0;
	std::string s;
	for (const auto &p : pos) {
		if (x > 0) {
			s += ",";
		}
		s += std::to_string(p);
		x++;
	}
	return s;
}

