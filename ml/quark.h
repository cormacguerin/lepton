#ifndef _QUARK_H_
#define _QUARK_H_

#include <string>
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
	void quit( int code );
	void init( std::string file );
	void trainByWords( std::string file );
	void trainBySentence( std::string file );
	std::string sanitizeText(std::string str);
	std::string toLowerCase(std::string str);
	bool isWord(std::string str);

};

#endif

