#ifndef _INDEXROOT_H_
#define _INDEXROOT_H_

#include <string>
#include <pqxx/pqxx>

class Indexroot {
	private:
		pqxx::connection* C;
		void adminConnect();

	public:
		Indexroot();
		~Indexroot();
    void run();
    int main();

};

#endif

