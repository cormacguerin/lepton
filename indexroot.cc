#include <iostream>
#include "proton.h"
//#include "quark.h"

using namespace std;

std::string port = "3333";

Proton proton;
//Quark quark;

int main(int argc, char** argv)
{
	//static const std::string lar[] = {"en","ja"};
	static const std::string lar[] = {"en"};
	std::vector<std::string> langs (lar, lar + sizeof(lar)/sizeof(lar[0]));

	std::string raw_query = "running up that hill";

	proton.init();
	// quark.init();

	for (vector<std::string>::iterator lit = langs.begin(); lit != langs.end(); ++lit) {
		proton.processFeeds(*lit);
	}
	// quark.buildIndex();

	proton.exportVocab("en");
	return 0;
}

