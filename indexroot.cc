#include <iostream>
#include "proton.h"

using namespace std;

std::string port = "3333";

Proton proton;

int main(int argc, char** argv)
{
	//static const std::string lar[] = {"en","ja"};
	static const std::string lar[] = {"en"};
	std::vector<std::string> langs (lar, lar + sizeof(lar)/sizeof(lar[0]));

	std::string raw_query = "running up that hill";

	proton.init();

	for (vector<std::string>::iterator lit = langs.begin(); lit != langs.end(); ++lit) {
		proton.processFeeds(*lit);
		proton.updateIdf(*lit);
	}

//	proton.exportVocab("en");
	return 0;
}

