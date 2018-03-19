#include "sentence_piece_processor.h"
#include <iostream>

SentencePieceProcessor::SentencePieceProcessor()
{
}

SentencePieceProcessor::~SentencePieceProcessor()
{
}

void SentencePieceProcessor::init() {
	//sentencepiece::SentencePieceProcessor processor;
	processor.LoadOrDie("word.model");
	//spec = processor.model_proto().normalizer_spec();
}

void SentencePieceProcessor::tokenize(std::string text, std::vector<std::string> *pieces) {
	//processor.Encode(normalizer.Normalize(text), &pieces);
	processor.Encode(text, pieces);
	/*
	for(std::vector<std::string>::iterator it = pieces.begin(); it != pieces.end(); ++it) {
		std::cout << " parsed_contents  b " << *it << std::endl;
	}
	*/
}

void SentencePieceProcessor::detokenize(std::vector<std::string> pieces, std::string text) {
	processor.Decode(pieces, &text);
//	for (const std::string &token : pieces) {
//		std::cout << token << std::endl;
//	}
}
