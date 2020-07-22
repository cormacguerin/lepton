#include "query_builder.h"

QueryBuilder::QueryBuilder()
{
#define N_GRAM_SIZE 3
#define IS_CJK false
	init();
}

QueryBuilder::~QueryBuilder()
{
}

void QueryBuilder::init() {
	
	//sentencepiece::QueryBuilder processor;
	//spec = processor.model_proto().normalizer_spec();

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

}


void QueryBuilder::build(std::string lang, std::string query_str, Query::Node &result) {

	// rootNode node, by default this is OR 
	Query::Node rootNode = {};
	rootNode.root = true;
	rootNode.op = Query::Operator::OR;
	rootNode.lang = lang;

	Query::Node branchNode = {};
	branchNode.root = false;
	branchNode.op = Query::Operator::AND;
	branchNode.lang = lang;

	Query::Node ngramNode = {};
	ngramNode.root = false;
	ngramNode.op = Query::Operator::OR;
	ngramNode.lang = lang;

	// temp containsers we use for processing
	std::map<std::vector<std::string>, int> gramCandidates;

	// convert to lowercase
	std::transform((query_str).begin(), (query_str).end(), (query_str).begin(), ::tolower);

	icu::UnicodeString uni_str = query_str.c_str();

	UErrorCode status = U_ZERO_ERROR;
	// BreakIterator *wordIterator = BreakIterator::createWordInstance(icu::Locale("ja","JAPAN"), status);
	icu::BreakIterator *wordIterator = icu::BreakIterator::createWordInstance(icu::Locale("en","US"), status);
	wordIterator->setText(uni_str);
	int32_t p = wordIterator->first();
	int32_t l = p;
	int i = 0;

	int ja_stop_words_count = 0;
	int en_stop_words_count = 0;

    std::vector<int> stoppattern;

    // process single terms
	while (p != icu::BreakIterator::DONE) {

		Query::Term term;
		term.type = Query::Type::ORIGINAL;
		term.gram = Query::Gram::UNIGRAM;
		
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
		
		icu::UnicodeString uc = icu::UnicodeString::fromUTF8(converted);
		term.term = uc;

		if ( std::find(ja_stop_words.begin(), ja_stop_words.end(), converted) != ja_stop_words.end() ) {
			isStopWord = true;
		}
		if ( std::find(en_stop_words.begin(), en_stop_words.end(), converted) != en_stop_words.end() ) {
			isStopWord = true;
		}

		std::cout << converted << " : " << isStopWord << std::endl;
		if (isStopWord == true) {
			Query::AttributeValue v;
			v.b=true;
			//term.mods.push_back(std::vector<std::pair<Query::Modifier,Query::AttributeValue>>(Query::Modifier::STOPWORD, v));
			term.mods.push_back(std::pair<Query::Modifier,Query::AttributeValue>(Query::Modifier::STOPWORD, v));
		    stoppattern.push_back(1);
		} else {
		    stoppattern.push_back(0);
        }

		branchNode.leafNodes.push_back(genTermNode(lang,term,query_str));
	}

    buildNgramNode(lang, branchNode, ngramNode, stoppattern);
	rootNode.leafNodes.push_back(ngramNode);
	rootNode.leafNodes.push_back(branchNode);

	result = rootNode;

	delete wordIterator;
}

void QueryBuilder::buildNgramNode(std::string lang, Query::Node branchNode, Query::Node &ngramNode, std::vector<int> &stoppattern) {
    // analyze stop pattern for bigram and ngram candidates.
    // our max query size is 1000 chars, so ignore anything above that
    //
    
    // an int to track the node position we are currently at
    // we erase stoppattern as we progress, so we need to incrememt position to ensure we are in sync with the nodes.
    int position = 0;
    bool isCJK;
    if (lang == "en") {
        isCJK = false;
    } else if (lang == "ja") {
        isCJK = true;
    }
    while (stoppattern.size() > 1) {
        // an int to track which pattern we are testing for (check header file)
        int pattern_case = 0;
        bool matched = false;
        for (std::vector<std::vector<int>>::const_iterator pit = ngram_patterns.begin(); pit != ngram_patterns.end(); pit++) {
            if (stoppattern.size() < 2) {
                return;
            }
            // skip patterns beginning with a stop word
            if (stoppattern.at(0) == 1) {
                stoppattern.erase(stoppattern.begin());
                position++;
            }
            int xx = findPattern(*pit,stoppattern);
            // this logic is based on the header file patterns
            if (xx < 1000) {
                // we found a match, erase up to the start of the match and increment position accordingly
                stoppattern.erase(stoppattern.begin(),stoppattern.begin()+xx);
                position = position + xx;
                matched = true;
                Query::Term ngterm;
                ngterm.type = Query::Type::ORIGINAL;
                switch(pattern_case) {
                    case 0 :
                      {
                        for (int i=position; i<(5+position); i++) {
                            ngterm.term.append(branchNode.leafNodes.at(i).term.term);
                            if (isCJK == false) {
                                ngterm.term.append(" ");
                            }
                        }
                        ngterm.term.trim();
		                ngterm.gram = Query::Gram::NGRAM;
                        ngramNode.leafNodes.push_back(genTermNode(lang,ngterm,query_str));
                        ngterm.term.remove();

                        for (int i=position; i<(3+position); i++) {
                            ngterm.term.append(branchNode.leafNodes.at(i).term.term);
                            if (isCJK == false) {
                                ngterm.term.append(" ");
                            }
                        }
                        ngterm.term.trim();
		                ngterm.gram = Query::Gram::TRIGRAM;
                        ngramNode.leafNodes.push_back(genTermNode(lang,ngterm,query_str));
                        ngterm.term.remove();

                        for (int i=(position+3); i<(5+position); i++) {
                            ngterm.term.append(branchNode.leafNodes.at(i).term.term);
                            if (isCJK == false) {
                                ngterm.term.append(" ");
                            }
                        }
                        ngterm.term.trim();
		                ngterm.gram = Query::Gram::BIGRAM;
                        ngramNode.leafNodes.push_back(genTermNode(lang,ngterm,query_str));
                        ngterm.term.remove();

                        for (int i=position; i<(2+position); i++) {
                            ngterm.term.append(branchNode.leafNodes.at(i).term.term);
                            if (isCJK == false) {
                                ngterm.term.append(" ");
                            }
                        }
                        ngterm.term.trim();
		                ngterm.gram = Query::Gram::BIGRAM;
                        ngramNode.leafNodes.push_back(genTermNode(lang,ngterm,query_str));
                        ngterm.term.remove();

                        for (int i=(position+2); i<(5+position); i++) {
                            ngterm.term.append(branchNode.leafNodes.at(i).term.term);
                            if (isCJK == false) {
                                ngterm.term.append(" ");
                            }
                        }
                        ngterm.term.trim();
		                ngterm.gram = Query::Gram::TRIGRAM;
                        ngramNode.leafNodes.push_back(genTermNode(lang,ngterm,query_str));
                        stoppattern.erase(stoppattern.begin(),stoppattern.begin()+ngram_patterns.at(0).size());
                        position = position + ngram_patterns.at(0).size();
                        ngterm.term.remove();
                        break;
                      }

                    case 1 :
                      {
                        for (int i=position; i<(4+position); i++) {
                            ngterm.term.append(branchNode.leafNodes.at(i).term.term);
                            if (isCJK == false) {
                                ngterm.term.append(" ");
                            }
                        }
                        ngterm.term.trim();
		                ngterm.gram = Query::Gram::NGRAM;
                        ngramNode.leafNodes.push_back(genTermNode(lang,ngterm,query_str));
                        ngterm.term.remove();

                        for (int i=position; i<(2+position); i++) {
                            ngterm.term.append(branchNode.leafNodes.at(i).term.term);
                            if (isCJK == false) {
                                ngterm.term.append(" ");
                            }
                        }
                        ngterm.term.trim();
		                ngterm.gram = Query::Gram::BIGRAM;
                        ngramNode.leafNodes.push_back(genTermNode(lang,ngterm,query_str));
                        ngterm.term.remove();

                        for (int i=1+position; i<(3+position); i++) {
                            ngterm.term.append(branchNode.leafNodes.at(i).term.term);
                            if (isCJK == false) {
                                ngterm.term.append(" ");
                            }
                        }
                        ngterm.term.trim();
		                ngterm.gram = Query::Gram::BIGRAM;
                        ngramNode.leafNodes.push_back(genTermNode(lang,ngterm,query_str));
                        ngterm.term.remove();

                        for (int i=(2+position); i<(4+position); i++) {
                            ngterm.term.append(branchNode.leafNodes.at(i).term.term);
                            if (isCJK == false) {
                                ngterm.term.append(" ");
                            }
                        }
                        ngterm.term.trim();
		                ngterm.gram = Query::Gram::BIGRAM;
                        ngramNode.leafNodes.push_back(genTermNode(lang,ngterm,query_str));
                        stoppattern.erase(stoppattern.begin(),stoppattern.begin()+ngram_patterns.at(1).size());
                        position = position + ngram_patterns.at(1).size();
                        ngterm.term.remove();
                        break;
                      }

                    case 2 :
                      {
                        for (int i=position; i<(3+position); i++) {
                            ngterm.term.append(branchNode.leafNodes.at(i).term.term);
                            if (isCJK == false) {
                                ngterm.term.append(" ");
                            }
                        }
                        ngterm.term.trim();
		                ngterm.gram = Query::Gram::TRIGRAM;
                        ngramNode.leafNodes.push_back(genTermNode(lang,ngterm,query_str));
                        stoppattern.erase(stoppattern.begin(),stoppattern.begin()+ngram_patterns.at(2).size());
                        position = position + ngram_patterns.at(2).size();
                        ngterm.term.remove();
                        break;
                      }

                    case 3 :                    
                      {
                        for (int i=position; i<(2+position); i++) {
                            ngterm.term.append(branchNode.leafNodes.at(i).term.term);
                            if (isCJK == false) {
                                ngterm.term.append(" ");
                            }
                        }
                        ngterm.term.trim();
		                ngterm.gram = Query::Gram::BIGRAM;
                        ngramNode.leafNodes.push_back(genTermNode(lang,ngterm,query_str));
                        stoppattern.erase(stoppattern.begin(),stoppattern.begin()+ngram_patterns.at(3).size());
                        position = position + ngram_patterns.at(3).size();
                        ngterm.term.remove();
                        break;
                      }

                    case 4 :
                      {
                        for (int i=position; i<(4+position); i++) {
                            ngterm.term.append(branchNode.leafNodes.at(i).term.term);
                            if (isCJK == false) {
                                ngterm.term.append(" ");
                            }
                        }
                        ngterm.term.trim();
		                ngterm.gram = Query::Gram::NGRAM;
                        ngramNode.leafNodes.push_back(genTermNode(lang,ngterm,query_str));
                        stoppattern.erase(stoppattern.begin(),stoppattern.begin()+ngram_patterns.at(4).size());
                        position = position + ngram_patterns.at(4).size();
                        ngterm.term.remove();
                        break;
                      }

                    case 5 :
                      {
                        for (int i=position; i<(3+position); i++) {
                            ngterm.term.append(branchNode.leafNodes.at(i).term.term);
                            if (isCJK == false) {
                                ngterm.term.append(" ");
                            }
                        }
                        ngterm.term.trim();
		                ngterm.gram = Query::Gram::TRIGRAM;
                        ngramNode.leafNodes.push_back(genTermNode(lang,ngterm,query_str));
                        stoppattern.erase(stoppattern.begin(),stoppattern.begin()+ngram_patterns.at(5).size());
                        position = position + ngram_patterns.at(5).size();
                        ngterm.term.remove();
                        break;
                      }
                }
            }
            pattern_case++;
        }
        if (matched == false && stoppattern.size() > 1) {
            std::cout << "no pattern match found" << std::endl;
            stoppattern.erase(stoppattern.begin());
            position++;
        } else {
            std::cout << "pattern match was found" << std::endl;
        }
    }
}

Query::Node QueryBuilder::genTermNode(std::string lang, Query::Term term, std::string query_str) {
		Query::Node termNode = {};
		termNode.root = false;
		termNode.lang = lang;
		termNode.term = term;
		termNode.raw_query = query_str;
        return termNode;
}

int QueryBuilder::findPattern(std::vector<int> pattern, std::vector<int> in) {
    auto s = std::search(std::begin(in), std::end(in), std::begin(pattern), std::end(pattern));
    if (s != std::end(in)) {
        return (s - in.begin());
    } else {
        return 1000;
    }
}
