#ifndef _SERVER_H_
#define _SERVER_H_

#include <asio.hpp>
#include "index_server.h"

class Server {
	private:
		const asio::ip::tcp::endpoint endpoint;
		asio::io_context io_context;
		asio::ip::tcp::acceptor acceptor_;
		void do_accept();
		std::shared_ptr<IndexServer> indexServer;

	public:
		Server(short port, std::string database, std::string table);
		~Server();
		void run();
};
#endif

