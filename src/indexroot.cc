#include <iostream>
#include <thread>
#include "indexroot.h"
#include "frag.h"
#include "util.h"

using namespace std;

std::string port = "3333";

IndexRoot::IndexRoot() {
    adminConnect();
}

IndexRoot::~IndexRoot()
{
}

void IndexRoot::adminConnect() {
	try {
		C = new pqxx::connection("dbname = admin user = postgres password = " + getDbPassword() + " hostaddr = 127.0.0.1 port = 5432");
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
void IndexRoot::process() {
  pqxx::work txn(*C);
  // SELECT all database table columns which have indexing enabled
  std::string statement = "SELECT DISTINCT databases.database, tables.tablename, _column, indexing FROM text_tables_index INNER JOIN databases ON databases.id = text_tables_index.database INNER JOIN tables ON tables.id = text_tables_index._table";
  C->prepare("get_tables_to_index", statement);

  pqxx::result r = txn.prepared("get_tables_to_index").exec();
  txn.commit();

  std::map<std::string, std::map<std::string, std::pair<std::string,std::string>>> tables;

  /*
   * format the output into a useful container
   * database -> table -> pair<indexing,columns to index>
   */
  for (pqxx::result::const_iterator row = r.begin(); row != r.end(); ++row) {
    const pqxx::field database = (row)[0];
    const pqxx::field table = (row)[1];
    const pqxx::field column = (row)[2];
    const pqxx::field indexing = (row)[3];
    if (!database.is_null() && !table.is_null() && !column.is_null()) {
      if (tables[database.c_str()][table.c_str()].second.empty()) {
        tables[database.c_str()][table.c_str()].second = column.c_str();
      } else {
        std::string newcol = "," + std::string(column.c_str());
        tables[database.c_str()][table.c_str()].second += newcol;
      }
      tables[database.c_str()][table.c_str()].first = indexing.c_str();
    }
  }
  for (std::map<std::string, std::map<std::string, std::pair<std::string,std::string>>>::iterator dit = tables.begin(); dit != tables.end(); dit++) {
    // std::cout << "indexroot.cc : run() "  << dit->first << std::endl;
    for (std::map<std::string, std::pair<std::string, std::string>>::iterator tit = dit->second.begin(); tit != dit->second.end(); tit++) {
      // std::cout << "indexroot.cc : run() - table : " << tit->first << std::endl;
      // std::cout << "indexroot.cc : run()   columns : " << (tit->second).second << std::endl;
      // std::cout << "indexroot.cc : run()   indexing : " << (tit->second).first << std::endl;
      // loop over existing index managers to see if there is one already for this database table
      bool exists = false;
      for (std::vector<IndexManager*>::iterator imit = ims.begin(); imit != ims.end(); imit++) {
          // test for this database/table
          if ((*imit)->database == dit->first && (*imit)->table == tit->first) {
              // indexing is enabled but the instance is set not the run (start it up)
              if ((tit->second).first == "t" && (*imit)->do_run == false) {
                std::cout << "indexroot.cc : - restart thread " << dit->first << " " << tit->first << std::endl;
                (*imit)->do_run = true;
                std::thread t_(std::bind(static_cast<void (IndexManager::*)()>(&IndexManager::spawnProcessFeeds), *imit));
                t_.detach();
              }
              // indexing is disabled but the instance is running (shut it down)
              if ((tit->second).first == "f" && (*imit)->do_run == true) {
                (*imit)->do_run = false;
              }
              exists = true;
          }
      }
      // if the instance does not exist and it's supposed to be running, then init/start it.
      if (exists == false && (tit->second).first == "t") {
          std::cout << "indexroot.cc : create new IndexManager - " << dit->first << " " << tit->first << std::endl;
          IndexManager* indexManager = new IndexManager(Frag::Type::UNIGRAM, Frag::Type::BIGRAM, Frag::Type::TRIGRAM, dit->first, tit->first, (tit->second).second);
          //indexManager.init(dit->first);
          //std::thread t_(std::bind(static_cast<void (IndexManager::*)()>(&IndexManager::processFeeds), indexManager));
          //std::thread t_(std::bind(&IndexManager::processFeeds, indexManager));
          //std::thread t_(std::bind(&IndexManager::init, *ims.back()));
          ims.push_back(indexManager);
          std::thread t_(std::bind(static_cast<void (IndexManager::*)()>(&IndexManager::spawnProcessFeeds), indexManager));
          t_.detach();
      }
    }
  }
}

int main(int argc, char** argv) {
  IndexRoot ir;
  while (true) {
    ir.process();
    usleep(5000000);
  }
  return 0;
}

