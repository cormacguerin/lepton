#ifndef _QUARK_H_
#define _QUARK_H_

#include <string>
#include "word2vec.h"
#include "neuron.h"

class Quark {
	private:
        std::vector<std::string> Split(const std::string& subject);
		std::vector<Neuron::Neuron*> neuron;
		int num_neurons;

	public:
	Quark();
	~Quark();
	void quit( int code );
	void init( std::string file );
	void train( std::string file );

};

#endif

