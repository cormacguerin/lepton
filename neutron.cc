#include <iostream>
#include <core/client.hpp>

#include "proton.h"
#include "quark.h"
#include "server.h"

using namespace std;

std::string port = "3333";

Proton proton;
Quark quark;
Server server(std::atoi(port.c_str()));

int main(int argc, char** argv)
{
	//static const std::string lar[] = {"en","ja"};
	static const std::string lar[] = {"en"};
	std::vector<std::string> langs (lar, lar + sizeof(lar)/sizeof(lar[0]));

	proton.init();
	quark.init();
	server.run();

	/*
	for (vector<std::string>::iterator lit = langs.begin(); lit != langs.end(); ++lit) {
		proton.processFeeds(*lit);
	}
	*/
	quark.buildIndex();

	//proton.exportVocab("en");
	return 0;
}

