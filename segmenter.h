#ifndef _SENTENCE_PIECE_PROCESSOR_H_
#define _SENTENCE_PIECE_PROCESSOR_H_

#include "sentence_piece_processor.h"

class Segmenter {
	private:

	public:
		Segmenter();
		~Segmenter();
		void init();
		void tokenize(std::string text, std::vector<std::string> *pieces);
		void detokenize(std::vector<std::string> pieces, std::string text);
};

#endif
