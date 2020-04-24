#include <iostream>
#include "serveroot.h"

using namespace std;

ServeRoot::ServeRoot() {
  adminConnect();
}

ServeRoot::~ServeRoot()
{
  for (std::vector<Server*>::iterator it = servers.begin(); it != servers.end(); it++) {
    delete *it;
  }
}

void ServeRoot::adminConnect() {
	try {
		C = new pqxx::connection("dbname = admin user = postgres password = kPwFWfYAsyRGZ6IomXLCypWqbmyAbK+gnKIW437QLjw= hostaddr = 127.0.0.1 port = 5432");
    if (C->is_open()) {
       cout << "Opened database successfully: " << C->dbname() << endl;
    } else {
       cout << "Can't open database" << endl;
    }
	} catch (const std::exception &e) {
		cerr << e.what() << std::endl;
		exit;
	}
}

/*
 * Run the main indexing process
 */
void ServeRoot::run() {
  pqxx::work txn(*C);
  // SELECT all database table columns which have indexing enabled
  std::string statement = "SELECT databases.database, tables.tablename, _column, display_field FROM text_tables_index INNER JOIN databases ON databases.id = text_tables_index.database INNER JOIN tables ON tables.id = text_tables_index._table WHERE serving = true";
  C->prepare("get_tables_to_serve", statement);

  pqxx::result r = txn.prepared("get_tables_to_serve").exec();
  txn.commit();

  std::map<std::string, std::map<std::string, std::pair<std::string,std::string>>> tables;

  /*
   * format the output into a useful container
   * database -> table -> pair<display_field,columns to index>
   */
  for (pqxx::result::const_iterator row = r.begin(); row != r.end(); ++row) {
    const pqxx::field database = (row)[0];
    const pqxx::field table = (row)[1];
    const pqxx::field column = (row)[2];
    const pqxx::field display_field = (row)[3];
    std::cout << "xx  " << database.c_str() << std::endl;
    std::cout << "xx  " << table.c_str() << std::endl;
    if (!database.is_null() && !table.is_null() && !column.is_null()) {
      if (tables[database.c_str()][table.c_str()].second.empty()) {
        tables[database.c_str()][table.c_str()].second = column.c_str();
      } else {
        std::string newcol = "," + std::string(column.c_str());
        tables[database.c_str()][table.c_str()].second += newcol;
      }
      tables[database.c_str()][table.c_str()].first = display_field.c_str();
    }
  }

  int port = 3333;
  for (std::map<std::string, std::map<std::string, std::pair<std::string,std::string>>>::iterator dit = tables.begin(); dit != tables.end(); dit++) {
    std::cout << "sereroot.cc : run() "  << dit->first << std::endl;
    for (std::map<std::string, std::pair<std::string, std::string>>::iterator tit = dit->second.begin(); tit != dit->second.end(); tit++) {
      std::cout << "indexroot.cc : run() - table : " << tit->first << std::endl;
      std::cout << "indexroot.cc : run()   columns : " << (tit->second).second << std::endl;
      std::cout << "indexroot.cc : run()   display_field : " << (tit->second).first << std::endl;
      servers.push_back(new Server(port++, dit->first, tit->first));
      // Server server(port++, dit->first, tit->first);
      servers.back()->run();
    }
  }
}

int main(int argc, char** argv) {
  ServeRoot sr;
  sr.run();
	return 0;
}

