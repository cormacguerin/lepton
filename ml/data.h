/*
 * Author : Cormac Guerin 
 *
 * Name: data.h
 *
 */

#ifndef _DATA_H_
#define _DATA_H_


#include <iostream>
#include <vector>

class Data {
	private:
		struct item {
			long hash;
			std::string content;
		};

		std::vector<item> items;
		std::vector<std::string> content;
		std::hash<std::string> hasher;

	public:
	Data();
	~Data();
		void add(std::string line);
		void addTrainData(std::vector<std::string> content);
		std::vector<std::string> getTrainData();

};

#endif

