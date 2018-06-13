#include <iostream>
#include <core/client.hpp>

#include "proton.h"

using namespace std;

Proton proton;

int main(int argc, char** argv)
{
	//static const std::string lar[] = {"en","ja"};
	static const std::string lar[] = {"en"};
	std::vector<std::string> langs (lar, lar + sizeof(lar)/sizeof(lar[0]));

	proton.init();

	for (vector<std::string>::iterator lit = langs.begin(); lit != langs.end(); ++lit) {
		proton.processFeeds(*lit);
	//	proton.exportVocab(*lit);
	}
	return 0;
}

