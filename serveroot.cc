#include <iostream>
#include "server.h"

using namespace std;

std::string port = "3333";

Server server(std::atoi(port.c_str()));

int main(int argc, char** argv)
{
	//static const std::string lar[] = {"en","ja"};
	static const std::string lar[] = {"en"};
	std::vector<std::string> langs (lar, lar + sizeof(lar)/sizeof(lar[0]));

	std::string raw_query = "running up that hill";

	server.run();

	return 0;
}

