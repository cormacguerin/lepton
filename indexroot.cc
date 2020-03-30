#include <iostream>
#include "indexroot.h"
#include "index_manager.h"
#include "frag.h"

using namespace std;

std::string port = "3333";

Indexroot::Indexroot() {
  adminConnect();
}

Indexroot::~Indexroot()
{
}

void Indexroot::adminConnect() {
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
void Indexroot::run() {
  pqxx::work txn(*C);
  // SELECT all database table columns which have indexing enabled
  std::string statement = "SELECT databases.database, tables.tablename, _column FROM text_tables_index INNER JOIN databases ON databases.id = text_tables_index.database INNER JOIN tables ON tables.id = text_tables_index._table  WHERE enable = true";
  C->prepare("get_tables_to_index", statement);

  pqxx::result r = txn.prepared("get_tables_to_index").exec();
  txn.commit();

  std::map<std::string, std::map<std::string, std::string>> tables;

  /*
   * format the output into a useful container
   */
  for (pqxx::result::const_iterator row = r.begin(); row != r.end(); ++row) {
    const pqxx::field database = (row)[0];
    const pqxx::field table = (row)[1];
    const pqxx::field column = (row)[2];
    if (!database.is_null() && !table.is_null() && !column.is_null()) {
      if (tables[database.c_str()][table.c_str()].empty()) {
        tables[database.c_str()][table.c_str()] = column.c_str();
      } else {
        std::string newcol = "," + std::string(column.c_str());
        tables[database.c_str()][table.c_str()] += newcol;
      }
    }
  }
  for (std::map<std::string, std::map<std::string, std::string>>::iterator dit = tables.begin(); dit != tables.end(); dit++) {
    std::cout << "indexroot.cc : run() "  << dit->first << std::endl;
    for (std::map<std::string, std::string>::iterator tit = dit->second.begin(); tit != dit->second.end(); tit++) {
      IndexManager indexManager(Frag::Type::UNIGRAM, Frag::Type::BIGRAM, Frag::Type::TRIGRAM, dit->first, tit->first);
      indexManager.init(dit->first);
      std::cout << "indexroot.cc : run() - table : " << tit->first << std::endl;
      std::cout << "indexroot.cc : run()   columns : " << tit->second << std::endl;
      indexManager.processFeeds(tit->first);
      indexManager.processDocInfo(tit->first);
    }
  }
}

  /*

	indexManager.init("test");

	for (vector<std::string>::iterator lit = langs.begin(); lit != langs.end(); ++lit) {
		indexManager.processFeeds(*lit);
		indexManager.processDocInfo(*lit);
	}
  */

//	indexManager.exportVocab("en");
int main(int argc, char** argv) {
  Indexroot ir;
  ir.run();
	return 0;
}

