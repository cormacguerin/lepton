#ifndef _QUERY_SERVER_H_
#define _QUERY_SERVER_H_

#include <asio.hpp>
#include "index_server.h"

class QueryServer {
	private:
		const asio::ip::tcp::endpoint endpoint;
		void do_accept();
		std::shared_ptr<IndexServer> indexServer;
    asio::io_context io_context;
    asio::ip::tcp::acceptor acceptor_;

	public:
    int port;
    std::string status;
    std::string table;
    std::string database;
    std::string do_query(std::string body);
		QueryServer(short port, std::string database, std::string table);
		~QueryServer();
		void run();
    std::map<std::string,int> getPercentLoaded();
    std::map<std::string,int> getServingInfo();
};
#endif

