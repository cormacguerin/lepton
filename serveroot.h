#ifndef _SERVEROOT_H_
#define _SERVEROOT_H_

#include <string>
#include <pqxx/pqxx>

class ServeRoot {
	private:
		pqxx::connection* C;
		void adminConnect();

	public:
		ServeRoot();
		~ServeRoot();
    void run();
    int main();

};

#endif

