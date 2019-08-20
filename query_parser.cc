#include "query_parser.h"
#include "query.h"

QueryParser::QueryParser()
{
#define N_GRAM_SIZE 3
#define IS_CJK false
}

QueryParser::~QueryParser()
{
}

void QueryParser::init() {
	
	//sentencepiece::QueryParser processor;
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


void QueryParser::parse(std::string lang, std::string query_str, std::string &result) {

	// rootNode node, by default this is OR 
	Query::Node rootNode = {};
	rootNode.root = true;
	rootNode.op = Query::Operator::OR;
	rootNode.raw_query = query_str;
	rootNode.lang = lang;

	Query::Node termNode = {};
	termNode.root = false;
	termNode.op = Query::Operator::AND;
	termNode.raw_query = query_str;
	termNode.lang = lang;

	// temp containsers we use for processing
	std::map<std::vector<std::string>, int> gramCandidates;

	// convert to lowercase
	std::transform((query_str).begin(), (query_str).end(), (query_str).begin(), ::tolower);

	UnicodeString uni_str = query_str.c_str();

	UErrorCode status = U_ZERO_ERROR;
	// BreakIterator *wordIterator = BreakIterator::createWordInstance(Locale("ja","JAPAN"), status);
	BreakIterator *wordIterator = BreakIterator::createWordInstance(Locale("en","US"), status);
	wordIterator->setText(uni_str);
	int32_t p = wordIterator->first();
	int32_t l = p;
	int i = 0;

	int ja_stop_words_count = 0;
	int en_stop_words_count = 0;

	std::vector<std::string> gramholder[N_GRAM_SIZE];
	std::vector<bool> stopholder[N_GRAM_SIZE];

	while (p != BreakIterator::DONE) {

		Query::Term term;
		term.type = Query::Type::ORIGINAL;
		
		bool isStopWord = false;
		p = wordIterator->next();
		std::string converted;
		UnicodeString tmp = uni_str.tempSubString(l,p-l);
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
		
		UnicodeString uc = UnicodeString::fromUTF8(converted);
		term.term = uc;
		term.idf = 0.0;

		if ( std::find(ja_stop_words.begin(), ja_stop_words.end(), converted) != ja_stop_words.end() ) {
			isStopWord = true;
		}
		if ( std::find(en_stop_words.begin(), en_stop_words.end(), converted) != en_stop_words.end() ) {
			isStopWord = true;
		}
		if (isStopWord == true) {
			Query::AttributeValue v;
			v.b=true;
			term.mods.insert(std::pair<Query::Modifier,Query::AttributeValue>(Query::Modifier::STOPWORD, v));
		}

		Query::Node leafNode = {};
		leafNode.root = false;
		leafNode.op = Query::Operator::OR;
		leafNode.raw_query = query_str;
		leafNode.lang = lang;
		//leafNode.terms.push_back(term);
		leafNode.term = term;
		termNode.leafNodes.push_back(leafNode);

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

	rootNode.leafNodes.push_back(termNode);

	result = rootNode.serialize();

	delete wordIterator;
}

void QueryParser::execute(std::string lang, std::string query, std::promise<std::string> *promiseObj) {
	std::string result;
	QueryParser qp;
	qp.parse(lang, query, result);
	promiseObj->set_value(result);
}

