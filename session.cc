#include "session.h"
#include <set>
#include <utility>
#include <iostream>
#include <thread>
#include <future>
#include <memory>


//Session::Session(asio::ip::tcp::socket socket, std::shared_ptr<IndexServer> indexServer) : socket_(std::move(socket)), is_(std::move(indexServer))
Session::Session(asio::ip::tcp::socket socket) : socket_(std::move(socket))
{
}

Session::~Session() 
{
}

request<char*> req;
response<std::string> res;

//void Session::start(IndexServer *indexServer) {
void Session::start(const std::shared_ptr<IndexServer> &indexServer) {
	is_ = indexServer;
	if (req.body == NULL) {
		std::cout << "req is null " << std::endl;
	} else {
		std::cout << "session.cc : req length " << strlen(req.body) << std::endl;
	}
	do_read_header();
}

void Session::do_read_header() {
	auto self(shared_from_this());

	asio::async_read(socket_,
			asio::buffer(req.header, (std::size_t)req.header_length),
			[this, self](std::error_code ec, std::size_t) {
				if (!ec) {
					req.decode_message();
					do_read_body();
				} else {
					std::cout << ec << std::endl;
					return;
				}
	});
}

void Session::do_read_body() {

	auto self(shared_from_this());
	asio::async_read(socket_,
			asio::buffer(req.body, (std::size_t)req.body_length),
			[this, self](std::error_code ec, std::size_t) {
				if (!ec) {
					std::string lang="en";
					std::promise<std::string> promiseObj;
					std::future<std::string> futureObj = promiseObj.get_future();
					is_.get()->execute(lang, std::string(req.body), std::move(promiseObj));
					do_write(futureObj.get());
				} else {
					std::cout << ec << std::endl;
				}
				free(req.body);
	});
}

void Session::do_write(std::string response) {
	// res.encode_message(const_cast<char*>(response));
	// std::cout << "session.cc : res.body_length " << res.body_length << std::endl;
	auto self(shared_from_this());
	asio::async_write(socket_,
			asio::buffer(response, response.length()),
			[this, self](std::error_code ec, std::size_t /*length*/) {
				if (!ec) {
					// std::cout << "session.cc : body - " << res.body << std::endl;
				}
	});
}

