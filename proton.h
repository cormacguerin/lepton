#ifndef _PROTON_H_
#define _PROTON_H_

#include <string>
//#include "sentence_piece_processor.h"
#include "segmenter.h"

class Proton {
	private:
		// SentencePieceProcessor spp;
		Segmenter seg;
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
	void indexDocument(std::string key, std::string doc);

};

#endif

