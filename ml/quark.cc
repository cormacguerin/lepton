#include <iostream>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iterator>
#include <functional>
#include "quark.h"

using namespace std;

Quark::Quark()
{
	num_neurons = 300;
}

Quark::~Quark()
{
}

Quark quark;
Word2Vec word2Vec;
Data data;

void Quark::init(string corefile) {

	for (int i = 0; i < num_neurons; i++) {
		neuron.push_back(new Neuron::Neuron());
	}

	for(std::vector<Neuron::Neuron*>::iterator it = neuron.begin(); it != neuron.end(); ++it) {
      		word2Vec.initNeuron((*it)->syn_in, num_neurons);
			//cout << (*it)->syn_in << endl;
	}
	
	ifstream infile (corefile);
	if (infile.is_open()) {
	        string line;
   	        while ( getline (infile, line) ) {
            		data.add(line);
   	        }
   	        infile.close();
  	}
}

void Quark::train(string trainfile) {
	ifstream infile (trainfile);
	if (infile.is_open()) {
	        string line;
   	        while ( getline (infile, line) ) {
            		word2Vec.processline(Split(line));
   	        }
   	        infile.close();
	}
}

int main(int argc, char** argv) {

	typedef float real;
	unsigned long long next_random = 1;
	for (int a = 0; a < 10; a++) for (int b = 0; b < 100; b++) {
		next_random = next_random * (unsigned long long)25214903917 + 11;
		float x = (((next_random & 0xFFFF) / (real)65536) - 0.5) / 100;
	}
	if (argc == 1) {
		return 1;
	}
	quark.init(argv[1]);
	quark.train(argv[2]);
	return 0;
}

std::vector<std::string> Quark::Split(const std::string& subject)
{
    std::istringstream ss{subject};
    using StrIt = std::istream_iterator<std::string>;
    std::vector<std::string> container{StrIt{ss}, StrIt{}};
    return container;
}

