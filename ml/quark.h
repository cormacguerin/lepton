#ifndef _QUARK_H_
#define _QUARK_H_

#include <map>
#include <set>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iterator>
#include <functional>
#include <algorithm>
#include "word2vec.h"
#include "neuron.h"

class Quark {
	private:
        std::vector<std::string> Split(const std::string& subject);
		std::vector<Neuron::Neuron*> neurons;
		int num_neurons;
        int data_size;

	public:
	Quark();
	~Quark();
	std::map<std::string,int> *vocab;
	std::set<std::string> common_words;
	void quit( int code );
	void init( std::string str );
	void trainByWords( std::string str );
	void trainBySentence( std::string str );
	void readCommonWords(std::string str);
	std::string sanitizeText(std::string str);
	std::string toLowerCase(std::string str);
	bool isWord(std::string str);

};

#endif

