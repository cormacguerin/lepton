#ifndef _INDEXROOT_H_
#define _INDEXROOT_H_

#include <string>
#include <pqxx/pqxx>

class IndexRoot {
	private:
		pqxx::connection* C;
		void adminConnect();

	public:
		IndexRoot();
		~IndexRoot();
    void run();
    int main();

};

#endif

