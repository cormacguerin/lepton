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
  //run();
}

QueryServer::~QueryServer()
{
}

void QueryServer::do_accept() {
	acceptor_.async_accept([this](std::error_code ec, asio::ip::tcp::socket socket) {
		if (!ec) {
			std::cout << "accept" << std::endl;
      std::shared_ptr sptr = std::make_shared<Session>(std::move(socket));
      sptr->set_callback(std::bind(&QueryServer::do_query, this, std::placeholders::_1));
      sptr->do_read_header();
		} else {
			std::cout << "EC" << std::endl;
			std::cout << ec << std::endl;
		}
		do_accept();
	});
}

std::string QueryServer::do_query(std::string body) {
	rapidjson::Document parsed_query;
	parsed_query.Parse(body.c_str());
  std::string lang = parsed_query.FindMember("lang")->value.GetString();
  std::string query = parsed_query.FindMember("query")->value.GetString();
  std::promise<std::string> promiseObj;
  std::future<std::string> futureObj = promiseObj.get_future();
  indexServer.get()->execute(lang, query, std::move(promiseObj));
  return futureObj.get();
}

// we don't access shared resources so I think we shouldn't need the full implimentation. more at 
// https://www.boost.org/doc/libs/1_69_0/doc/html/boost_asio/tutorial/tuttimer5.html
void QueryServer::run() {
  indexServer.get()->init();
	do_accept();
  io_context.run();
	std::cout << "Run QueryServer thread for " << database << " - " << table << std::endl;
  // printer p(io);
	//do_accept();
  //io_context.run();
  /*
  std::thread t(std::bind(static_cast<size_t (asio::io_context::*)()>(&asio::io_context::run), &io_context));
  t.detach();
  */
}

std::map<std::string,int> QueryServer::getServingInfo() {
  std::cout << "CORMAC " << std::endl;
  return indexServer->getServingInfo();
}

std::map<std::string,int> QueryServer::getPercentLoaded() {
  return indexServer->getPercentLoaded();
}
