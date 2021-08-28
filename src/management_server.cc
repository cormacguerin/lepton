#include "management_server.h"
#include "session.h"
#include "message.h"
#include <string>
#include <iostream>
#include <memory>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include "util.h"

ManagementServer::ManagementServer(short port) : acceptor_(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
{
    init();
    do_accept();
}

ManagementServer::~ManagementServer()
{
    for (std::vector<QueryServer*>::iterator it = servers.begin(); it != servers.end(); it++) {
        delete *it;
    }
    C->close();
    delete C;
}

void ManagementServer::init() {
    auto config = getConfig();
    try {
		    C = new pqxx::connection("dbname = " + config.postgres_database + " user = " + config.postgres_user + " password = " + config.postgres_password + " hostaddr = " + config.postgres_host + " port = " + config.postgres_port);
        if (C->is_open()) {
            std::cout << "Opened database successfully: " << C->dbname() << std::endl;
        } else {
            std::cout << "Can't open database" << std::endl;
        }
    } catch (const std::exception &e) {
        std::cerr << e.what() << std::endl;
        exit;
    }
    // SELECT all database table columns which have serving enabled
    std::string statement = "SELECT databases.database, tables.tablename, _column, display_field FROM text_tables_index INNER JOIN databases ON databases.id = text_tables_index.database INNER JOIN tables ON tables.id = text_tables_index._table WHERE serving = true";
    C->prepare("get_tables_to_serve", statement);
}

void ManagementServer::do_accept() {
    acceptor_.async_accept([this](std::error_code ec, asio::ip::tcp::socket socket) {
        if (!ec) {
            std::cout << "accept" << std::endl;
            std::shared_ptr sptr = std::make_shared<Session>(std::move(socket));
            sptr->set_callback(std::bind(&ManagementServer::do_management, this, std::placeholders::_1));
            sptr->do_read_header();
        } else {
            std::cout << "management_server.cc EC" << std::endl;
            std::cout << ec << std::endl;
        }
        do_accept();
    });
}

std::string ManagementServer::do_management(std::string body) {
    rapidjson::Document parsed_query;
    parsed_query.Parse(body.c_str());
    std::string query = parsed_query.FindMember("query")->value.GetString();
    std::cout << "management_server.cc query : " << query << std::endl;
    if (query == std::string("stats")) {
        return getStats();
    } else if (query == std::string("toggle_serving")) {
        std::string database = parsed_query.FindMember("database")->value.GetString();
        std::string table = parsed_query.FindMember("table")->value.GetString();
        std::string action = parsed_query.FindMember("action")->value.GetString();
        std::cout << "management_server.cc toggleServing : " << database << " " << table << " " << action << std::endl;
        return toggleServing(database,table,action);
    } else {
        return query;
    }
}

void ManagementServer::run() {
    std::cout << "Run ManagementServer." << std::endl;
    pqxx::work txn(*C);

    pqxx::result r = txn.exec_prepared("get_tables_to_serve");
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
        std::cout << "management_server.cc : run() "  << dit->first << std::endl;
        for (std::map<std::string, std::pair<std::string, std::string>>::iterator tit = dit->second.begin(); tit != dit->second.end(); tit++) {
            std::cout << "management_server.cc : run() - table : " << tit->first << std::endl;
            std::cout << "management_server.cc : run()   columns : " << (tit->second).second << std::endl;
            std::cout << "management_server.cc : run()   display_field : " << (tit->second).first << std::endl;
            servers.push_back(new QueryServer(port++, dit->first, tit->first));
        }
    }

    for (std::vector<QueryServer*>::iterator it = servers.begin(); it != servers.end(); it++) {
        std::thread t(std::bind(static_cast<void (QueryServer::*)()>(&QueryServer::run), *it));
        t.detach();
    }

    // start the management interface
    // io_context.run();
    // detach the asio thread so it can respond.
    std::thread t(std::bind(static_cast<size_t (asio::io_context::*)()>(&asio::io_context::run), &io_context));
    t.join();
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
        si.AddMember("status", rapidjson::Value(((*it)->getServingStatus()).c_str(), allocator).Move(), allocator);

        rapidjson::Value si_(rapidjson::kObjectType);
        std::map<std::string,int> s_map = (*it)->getServingInfo();
        for (std::map<std::string,int>::iterator it_ = s_map.begin(); it_ != s_map.end(); it_++) {
            si_.AddMember(rapidjson::Value(it_->first.c_str(), allocator).Move(), rapidjson::Value(std::to_string(it_->second).c_str(), allocator).Move(), allocator);
        }
        si.AddMember("terms", si_, allocator);

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

std::string ManagementServer::toggleServing(std::string database, std::string table, std::string action) {
    for (std::vector<QueryServer*>::iterator it = servers.begin(); it != servers.end(); it++) {
      if ((*it)->database == database && (*it)->table == table) {
        std::cout << "AAA action " << action << std::endl;
        if (action == "serving") {
          (*it)->stop();
        } else if (action == "shutdown") {
          (*it)->run();
        }
        std::string status = "{\"status\":\"" + (*it)->getServingStatus() + "\"}";
      }
    }
    // reckless
    servers.push_back(new QueryServer(port++, database, table));
    return "{\"status\":\"loading\"}";
}

/*
   for (std::vector<QueryServer*>::iterator it = servers.begin(); it != servers.end(); it++) {
   delete *it;
   }
   */
