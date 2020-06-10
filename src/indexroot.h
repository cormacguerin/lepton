#ifndef _INDEXROOT_H_
#define _INDEXROOT_H_

#include <string>
#include <pqxx/pqxx>
#include "index_manager.h"

class IndexRoot {
	private:
		pqxx::connection* C;
		void adminConnect();
        std::vector<IndexManager*> ims;

	public:
		IndexRoot();
		~IndexRoot();
        void process();
        int main();

};

#endif

