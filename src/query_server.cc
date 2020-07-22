#include "query_server.h"
#include "session.h"
#include "message.h"
#include <string>
#include <iostream>
#include <memory>
#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

QueryServer::QueryServer(short p, std::string d, std::string t) : acceptor_(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), p)) {
    port = p;
    database = d;
    table = t;
    indexServer = std::make_shared<IndexServer>(database, table);
}

QueryServer::~QueryServer()
{
}

void QueryServer::do_accept() {
    acceptor_.async_accept([this](std::error_code ec, asio::ip::tcp::socket socket) {
        if (!ec) {
            std::cout << "query_server.cc accept" << std::endl;
            std::shared_ptr sptr = std::make_shared<Session>(std::move(socket));
            sptr->set_callback(std::bind(&QueryServer::do_query, this, std::placeholders::_1));
            sptr->do_read_header();
        } else {
            std::cout << "query_server.cc EC" << std::endl;
            std::cout << ec << std::endl;
        }
        do_accept();
    });
}

std::string QueryServer::do_query(std::string body) {
    rapidjson::Document parsed_query;
    std::cout << " DEBUG query_server.cc - body.length() " << body.length() << std::endl;
    const char* m = body.c_str();
    parsed_query.Parse(body.c_str());

    std::string lang, query, filter, type;
    
    std::cout << "query_server.cc DEB 0 " << body <<std::endl;
    std::cout << body.c_str() <<std::endl;
    rapidjson::Value::ConstMemberIterator lit = parsed_query.FindMember("lang");
    std::cout << "query_server.cc DEB 1 " << body <<std::endl;
    if (lit != parsed_query.MemberEnd()) {
        std::cout << "query_server.cc DEB 2 " << body <<std::endl;
        lang = lit->value.GetString();
    } else {
        std::cout << "query_server.cc unable to parse query lang " << std::endl;
    }
    std::cout << "query_server.cc DEB 3 " << body <<std::endl;
    rapidjson::Value::ConstMemberIterator tit = parsed_query.FindMember("type");
    std::cout << "query_server.cc DEB 4 " << body <<std::endl;
    if (tit != parsed_query.MemberEnd()) {
    std::cout << "query_server.cc DEB 5 " << body <<std::endl;
        type = tit->value.GetString();
    } else {
        std::cout << "query_server.cc unable to parse query type " << std::endl;
    }
    rapidjson::Value::ConstMemberIterator qit = parsed_query.FindMember("query");
    if (qit != parsed_query.MemberEnd()) {
        query = qit->value.GetString();
    } else {
        std::cout << "query_server.cc unable to parse query query " << std::endl;
    }
    rapidjson::Value::ConstMemberIterator fit = parsed_query.FindMember("filter");
    if (fit != parsed_query.MemberEnd()) {
        filter = fit->value.GetString();
    } else {
        std::cout << "query_server.cc unable to parse query filter " << std::endl;
    }

    std::promise<std::string> promiseObj;
    std::cout << " DEBUG query_server.cc - about to get futureObj " << std::endl;
    std::future<std::string> futureObj = promiseObj.get_future();
    indexServer.get()->execute(lang, type, query, filter, std::move(promiseObj));
    std::cout << " DEBUG query_server.cc - done indexServer.get " << std::endl;
    return futureObj.get();
}

// https://www.boost.org/doc/libs/1_69_0/doc/html/boost_asio/tutorial/tuttimer5.html
void QueryServer::run() {
    do_accept();
    std::cout << "Run QueryServer thread for " << database << " - " << table << std::endl;
    // printer p(io);
    // do_accept();
    // io_context.run();
    std::thread t(std::bind(static_cast<size_t (asio::io_context::*)()>(&asio::io_context::run), &io_context));
    t.detach();

    // start the index server (loads the index)
    indexServer.get()->run();
}

std::map<std::string,int> QueryServer::getServingInfo() {
    std::cout << "CORMAC " << std::endl;
    return indexServer->getServingInfo();
}

std::map<std::string,int> QueryServer::getPercentLoaded() {
    return indexServer->getPercentLoaded();
}

std::string QueryServer::getServingStatus() {
    return indexServer->getServingStatus();
}
