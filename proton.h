#ifndef _PROTON_H_
#define _PROTON_H_

#include <string>
#include "sentence_piece_processor.h"

class Proton {
	private:
		SentencePieceProcessor spp;
		char *SPS;
		std::string SPC;

	public:
	Proton();
	~Proton();
	void quit( int code );
	void init();
	void processFeeds();
	void processVocab();
	bool isSPS(char firstchar);
	void indexDocument(std::string key, std::string doc);

};

#endif

