#include "management_server.h"
#include "session.h"
#include "message.h"
#include <string>
#include <iostream>
#include <memory>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

ManagementServer::ManagementServer(short port) : acceptor_(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
{
  adminConnect();
	do_accept();
}

ManagementServer::~ManagementServer()
{
  for (std::vector<QueryServer*>::iterator it = servers.begin(); it != servers.end(); it++) {
    delete *it;
  }
}

void ManagementServer::adminConnect() {
	try {
		C = new pqxx::connection("dbname = admin user = postgres password = kPwFWfYAsyRGZ6IomXLCypWqbmyAbK+gnKIW437QLjw= hostaddr = 127.0.0.1 port = 5432");
    if (C->is_open()) {
      std::cout << "Opened database successfully: " << C->dbname() << std::endl;
    } else {
      std::cout << "Can't open database" << std::endl;
    }
	} catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
		exit;
	}
}

void ManagementServer::do_accept() {
	acceptor_.async_accept([this](std::error_code ec, asio::ip::tcp::socket socket) {
		if (!ec) {
			std::cout << "accept" << std::endl;
      std::shared_ptr sptr = std::make_shared<Session>(std::move(socket));
      sptr->set_callback(std::bind(&ManagementServer::do_management, this, std::placeholders::_1));
      sptr->do_read_header();
		} else {
			std::cout << "EC" << std::endl;
			std::cout << ec << std::endl;
		}
		do_accept();
	});
}

std::string ManagementServer::do_management(std::string body) {
	rapidjson::Document parsed_query;
	parsed_query.Parse(body.c_str());
  std::string query = parsed_query.FindMember("query")->value.GetString();
	std::cout << "ManagementServer.cc query : " << query << std::endl;
	std::cout << "ManagementServer.cc getStats : " << getStats() << std::endl;
  if (query == std::string("stats")) {
    return getStats();
  } else {
    return query;
  }
}

void ManagementServer::run() {
	std::cout << "Run ManagementServer." << std::endl;
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

  /*
   * spin up a new connection port for each instance.
   */
  for (std::map<std::string, std::map<std::string, std::pair<std::string,std::string>>>::iterator dit = tables.begin(); dit != tables.end(); dit++) {
    std::cout << "sereroot.cc : run() "  << dit->first << std::endl;
    for (std::map<std::string, std::pair<std::string, std::string>>::iterator tit = dit->second.begin(); tit != dit->second.end(); tit++) {
      std::cout << "indexroot.cc : run() - table : " << tit->first << std::endl;
      std::cout << "indexroot.cc : run()   columns : " << (tit->second).second << std::endl;
      std::cout << "indexroot.cc : run()   display_field : " << (tit->second).first << std::endl;
      //std::thread t(std::bind(static_cast<void (QueryServer::*)()>(&QueryServer::run), *it));
      //t.detach();
      servers.push_back(new QueryServer(port++, dit->first, tit->first));
      // std::thread t(std::bind(&ManagementServer::startServerThread, this, port++, dit->first, tit->first));
      std::cout <<"MEH3" << servers.size()<< std::endl;
      // t.detach();
      std::cout <<"MEH4" << servers.size()<< std::endl;
 //     sptr->set_callback(std::bind(&ManagementServer::do_management, this, std::placeholders::_1));
 //     servers.back()->run();
    }
  }

  for (std::vector<QueryServer*>::iterator it = servers.begin(); it != servers.end(); it++) {
    std::thread t(std::bind(static_cast<void (QueryServer::*)()>(&QueryServer::run), *it));
    std::cout << "DEBUG *it->database" << (*it)->database << std::endl;
    //std::thread t(std::bind(&QueryServer::run, *it));
    t.detach();
  }

  // start the management interface
	io_context.run();
  // detach the asio thread so it can respond.
  // std::thread t(std::bind(static_cast<size_t (asio::io_context::*)()>(&asio::io_context::run), &io_context));
  // t.join();
}

void ManagementServer::startServerThread(int port, std::string database, std::string table) {
  std::cout <<"MEH1" << servers.size()<< std::endl;
  std::cout <<"MEH2" << servers.size()<< std::endl;
}

std::string ManagementServer::getStats() {
  std::cout << "getStats" << std::endl;
	rapidjson::Document stats;
	rapidjson::Document::AllocatorType& allocator = stats.GetAllocator();
	stats.Parse("{}");
  rapidjson::Value serverinfo(rapidjson::kArrayType);
  std::cout << "servers.size() " << servers.size() <<std::endl;

  for (std::vector<QueryServer*>::iterator it = servers.begin(); it != servers.end(); it++) {
	  rapidjson::Value si(rapidjson::kObjectType);
		si.AddMember("port", rapidjson::Value(std::to_string((*it)->port).c_str(), allocator).Move(), allocator);
		si.AddMember("database", rapidjson::Value(((*it)->database).c_str(), allocator).Move(), allocator);
		si.AddMember("table", rapidjson::Value(((*it)->table).c_str(), allocator).Move(), allocator);

	  rapidjson::Value si_(rapidjson::kObjectType);
    std::map<std::string,int> s_map = (*it)->getServingInfo();
    for (std::map<std::string,int>::iterator it_ = s_map.begin(); it_ != s_map.end(); it_++) {
		  si_.AddMember(rapidjson::Value(it_->first.c_str(), allocator).Move(), rapidjson::Value(std::to_string(it_->second).c_str(), allocator).Move(), allocator);
    }
		si.AddMember("status", si_, allocator);

	  rapidjson::Value si__(rapidjson::kObjectType);
    std::cout << "get percent loaded" << std::endl;
    std::map<std::string,int> l_map = (*it)->getPercentLoaded();
    for (std::map<std::string,int>::iterator it_ = l_map.begin(); it_ != l_map.end(); it_++) {
		  si__.AddMember(rapidjson::Value(it_->first.c_str(), allocator).Move(), rapidjson::Value(std::to_string(it_->second).c_str(), allocator).Move(), allocator);
    }
		si.AddMember("loaded", si__, allocator);

	  serverinfo.PushBack(si, allocator);
  }
  stats.AddMember("servers",serverinfo,allocator);
  // stringify
	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	stats.Accept(writer);
  return std::string(buffer.GetString());
}

/*
  for (std::vector<QueryServer*>::iterator it = servers.begin(); it != servers.end(); it++) {
    delete *it;
  }
  */