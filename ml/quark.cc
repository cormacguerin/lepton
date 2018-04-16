#include <iostream>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iterator>
#include <functional>
#include <algorithm>
#include "quark.h"

using namespace std;

Quark::Quark()
{
	num_neurons = 300;
	data_size = 100;
	vocab = new std::vector<string>();
}

Quark::~Quark()
{
}

Quark quark;
Word2Vec word2Vec;

void Quark::init(std::string vocabfile) {

	ifstream word_dict(vocabfile);
	if (word_dict.is_open()) {
		string line;
		while (getline(word_dict, line)) {
			// extract the word (removing any weight / other componetns after)
	//		line.erase(std::find(line.begin(), line.end(), '\t'), line.end());
			line = toLowerCase(line);
			if (isWord(line)) {
				line = sanitizeText(line);
				vocab->push_back(line);
			}
		}
	} else {
		cout << "no vocab file, bailing!" << endl;
	}

	for (int i = 0; i < num_neurons; i++) {
		neuron.push_back(new Neuron::Neuron());
		neuron.at(i)->init(vocab);
	}

	for(std::vector<Neuron::Neuron*>::iterator it = neuron.begin(); it != neuron.end(); ++it) {
    		Neuron::Neuron* p;
            if (it != neuron.begin()) {
                    p = *std::prev(it);
            }
      		word2Vec.initNeuron(num_neurons, *it, p);
	}

}

std::string Quark::sanitizeText(std::string str) {
	auto isPunct = [](char c) { 
		return std::ispunct(static_cast<unsigned char>(c));
	};
	auto isDigit = [](char c) { 
		return std::isdigit(static_cast<unsigned char>(c));
	};

	// remove punctuations
	str.erase(std::remove_if(str.begin(), str.end(), isPunct), str.end());
	// remove digits
	str.erase(std::remove_if(str.begin(), str.end(), isDigit), str.end());
	return str;
}

bool Quark::isWord(std::string str) {
	for (int i=0; i < str.length(); i++) {
		if (isdigit(str.at(i))) {
			return false;
		}
		if (ispunct(str.at(i))) {
			return false;
		}
	}
	return true;
}

// convert to lowercase
std::string Quark::toLowerCase(std::string str) {
	std::transform(str.begin(), str.end(), str.begin(), [](char c) {
			return std::tolower(static_cast<unsigned char>(c));
			});
	return str;
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
			std::string s(line);
			trainData.push_back(line);
		}
		infile.close();
	}
	for(std::vector<std::string>::iterator dit = trainData.begin(); dit != trainData.end(); ++dit) {
		for(std::vector<Neuron::Neuron*>::iterator nit = neuron.begin(); nit != neuron.end(); ++nit) {
			word2Vec.trainCBOW(*dit, *nit);
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

