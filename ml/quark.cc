#include <iostream>
#include <fstream>
#include <sstream>
#include <iostream>
#include <iterator>
#include "quark.h"

using namespace std;

Quark::Quark()
{
}

Quark::~Quark()
{
}

Quark quark;
Word2Vec word2Vec;

void Quark::init(string vocabfile) {
	ifstream infile (vocabfile);
	if (infile.is_open()) {
	        string line;
    	        while ( getline (infile, line) ) {
                      word2Vec.processline(Split(line));
    	        }
    	        infile.close();
  	}
}

int main(int argc, char** argv)
{
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
	return 0;
}

std::vector<std::string> Quark::Split(const std::string& subject)
{
    std::istringstream ss{subject};
    using StrIt = std::istream_iterator<std::string>;
    std::vector<std::string> container{StrIt{ss}, StrIt{}};
    return container;
}

