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
	data_size = 100;
}

Quark::~Quark()
{
}

Quark quark;
Word2Vec word2Vec;

void Quark::init(string corefile) {

	for (int i = 0; i < num_neurons; i++) {
		neuron.push_back(new Neuron::Neuron());
		neuron.at(i)->init(data_size);
	}

	for(std::vector<Neuron::Neuron*>::iterator it = neuron.begin(); it != neuron.end(); ++it) {
		Neuron::Neuron* p;
		if (it != neuron.begin()) {
			p = *std::prev(it);
		}
		//cout << (*it)->id << endl;
		//cout << p->id << endl;
		word2Vec.initNeuron(num_neurons, *it, p);
	}

	ifstream infile (corefile);
	if (infile.is_open()) {
		string line;
		while ( getline (infile, line) ) {
		}
		infile.close();
	}
}

// Iterate by words
void Quark::trainByWords(string trainfile) {
}

// Iterate by sentaces
void Quark::trainBySentence(string trainfile) {
	ifstream infile (trainfile);
	std::vector<std::string> trainData;
	if (infile.is_open()) {
		string line;
		while ( getline (infile, line) ) {
			cout << " test 1 " << line << endl;
			std::string s(line);
			trainData.push_back(line);
			cout << " test 2 " << endl;
		}
		cout << " test 3 " << endl;
		infile.close();
	}
	for(std::vector<std::string>::iterator dit = trainData.begin(); dit != trainData.end(); ++dit) {
		// split each sentence into a vector of words.
		std::stringstream ss(*dit);
		std::istream_iterator<std::string> begin(ss);
		std::istream_iterator<std::string> end;
		std::vector<std::string> ss_sentence(begin, end);
		for(std::vector<std::string>::iterator sit = ss_sentence.begin(); sit != ss_sentence.end(); ++sit) {
			for(std::vector<Neuron::Neuron*>::iterator nit = neuron.begin(); nit != neuron.end(); ++nit) {
				word2Vec.trainCBOW(*sit, *nit);
			}
		}
	}
}

int main(int argc, char** argv) {

	//	typedef float real;
	//	unsigned long long next_random = 1;
	//	for (int a = 0; a < 10; a++) for (int b = 0; b < 100; b++) {
	//		next_random = next_random * (unsigned long long)25214903917 + 11;
	//		float x = (((next_random & 0xFFFF) / (real)65536) - 0.5) / 100;
	//	}
	if (argc == 1) {
		return 1;
	}
	quark.init(argv[1]);
	quark.trainBySentence(argv[2]);
	return 0;
}

std::vector<std::string> Quark::Split(const std::string& subject)
{
	std::istringstream ss{subject};
	using StrIt = std::istream_iterator<std::string>;
	std::vector<std::string> container{StrIt{ss}, StrIt{}};
	return container;
}

