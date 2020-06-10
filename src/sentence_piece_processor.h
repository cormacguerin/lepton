#ifndef _SENTENCE_PIECE_PROCESSOR_H_
#define _SENTENCE_PIECE_PROCESSOR_H_

#include <sentencepiece_processor.h>

class SentencePieceProcessor {
	private:
		sentencepiece::SentencePieceProcessor processor;

	public:
		SentencePieceProcessor();
		~SentencePieceProcessor();
		void init();
		void tokenize(std::string text, std::vector<std::string> *pieces);
		void detokenize(std::vector<std::string> pieces, std::string text);
};

#endif
