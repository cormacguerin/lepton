#ifndef _SERVER_H_
#define _SERVER_H_

#include <asio.hpp>

class Server {
	private:
		const asio::ip::tcp::endpoint endpoint;
		asio::io_context io_context;
		asio::ip::tcp::acceptor acceptor_;
		void do_accept();

	public:
		Server(short port);
		~Server();
		void run();
};
#endif

