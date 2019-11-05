#include <iostream>
#include "index_manager.h"
#include "frag.h"

using namespace std;

std::string port = "3333";

IndexManager indexManager(Frag::Type::UNIGRAM, Frag::Type::BIGRAM, Frag::Type::TRIGRAM);

int main(int argc, char** argv)
{
	std::vector<std::string> langs = {"en"};

	std::string raw_query = "running up that hill";

	indexManager.init();

	for (vector<std::string>::iterator lit = langs.begin(); lit != langs.end(); ++lit) {
		indexManager.processFeeds(*lit);
		indexManager.processDocInfo(*lit);
	}

//	indexManager.exportVocab("en");
	return 0;
}

