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
		std::unique_ptr<IndexServer> indexServer;

	public:
		Server(short port);
		~Server();
		void run();
};
#endif

