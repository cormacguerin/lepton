#include <iostream>
#include <fstream>
#include <string>
#include "data.h"
#include <math.h>
#include <functional>

using namespace std;

Data::Data()
{
}

Data::~Data()
{
}

void Data::add(std::string indata) {
	item m;
	m.hash = hasher(indata);
	m.content = indata;
	(this->items).push_back(m);
	//(this->items).push_back(item(hasher(indata),indata));
	//cout << "this.hash " << this->hash << endl;
	//cout << "this.hash " << this->content << endl;
	return;
}

void Data::addTrainData(std::vector<std::string> content) {
	this->content = content;
}

std::vector<std::string> Data::getTrainData() {
	return this->content;
}
