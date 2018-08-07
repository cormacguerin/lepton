#ifndef _PROTON_H_
#define _PROTON_H_

#include <string>
//#include "sentence_piece_processor.h"
#include "segmenter.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <pqxx/pqxx>

class Proton {
	private:
		// SentencePieceProcessor spp;
		Segmenter seg;
		pqxx::connection* C;
		char *SPS;
		std::string SPC;

	public:
	Proton();
	~Proton();
	void quit( int code );
	void init();
	void processFeeds(std::string lang);
	void exportVocab(std::string lang);
	bool isSPS(char firstchar);
	void indexDocument(std::string id, std::string key, std::string doc, std::string lang);

};

#endif

