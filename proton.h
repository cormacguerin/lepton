#ifndef _PROTON_H_
#define _PROTON_H_

#include <string>
#include "sentence_piece_processor.h"

class Proton {
	private:
		SentencePieceProcessor spp;

	public:
	Proton();
	~Proton();
	void quit( int code );
	void init();
	void processFeeds();
	void processVocab();
	void indexDocument(std::string doc);

};

#endif

