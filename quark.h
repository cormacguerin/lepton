#ifndef _QUARK_H_
#define _QUARK_H_

#include <string>
#include "segmenter.h"
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include <pqxx/pqxx>

class Quark {
	private:
		pqxx::connection* C;
		std::map<int,std::vector<int>> gramsmap;
		bool serveing;

	public:
		Quark();
		~Quark();
		void init();
		void buildIndex();
		void startServing();

};

#endif

