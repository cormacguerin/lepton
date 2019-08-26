#include "server.h"
#include "session.h"
#include "message.h"
#include <string>
#include <iostream>
#include <memory>

Server::Server(short port) : acceptor_(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
{
	do_accept();
	indexServer = std::make_unique<IndexServer>();
}

Server::~Server()
{
}

void Server::do_accept() {
	acceptor_.async_accept([this](std::error_code ec, asio::ip::tcp::socket socket) {
		if (!ec) {
			std::cout << "accept" << std::endl;
			std::make_shared<Session>(std::move(socket), std::move(indexServer))->start();
		} else {
			std::cout << "EC" << std::endl;
			std::cout << ec << std::endl;
		}
		do_accept();
	});
}

void Server::run() {
	io_context.run();
}

