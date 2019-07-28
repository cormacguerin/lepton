#include "server.h"
#include "session.h"
#include <string>
#include <iostream>
#include <memory>

Server::Server(short port) : acceptor_(io_context, asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
{
	do_accept();
}

Server::~Server()
{
}

void Server::do_accept() {
	acceptor_.async_accept([this](std::error_code ec, asio::ip::tcp::socket socket) {
		if (!ec) {
			std::make_shared<Session>(std::move(socket))->start();
		}
	});
}

void Server::run() {
	io_context.run();
}

