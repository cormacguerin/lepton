#ifndef _SERVEROOT_H_
#define _SERVEROOT_H_

#include <string>
#include <pqxx/pqxx>
#include "server.h"

class ServeRoot {
	private:
		pqxx::connection* C;
		void adminConnect();
    std::vector<Server*> servers;

	public:
		ServeRoot();
		~ServeRoot();
    void run();
    int main();

};

#endif

