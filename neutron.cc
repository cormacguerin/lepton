#include <iostream>
#include <core/client.hpp>

#include "proton.h"

using namespace std;

Proton proton;

int main(int argc, char** argv)
{
	proton.init();
	proton.processFeeds();
	proton.processVocab();
	return 0;
}

