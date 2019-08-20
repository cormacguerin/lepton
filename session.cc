#include "session.h"
#include <memory>
#include <set>
#include <utility>
#include <iostream>
#include <thread>
#include <future>


Session::Session(asio::ip::tcp::socket socket) : socket_(std::move(socket))
{
}

Session::~Session() 
{
}


void Session::start() {
	std::cout << "session start" << std::endl;
	do_read_header();
}


/*
void deliver(const message& msg) {
	// do something here
}
*/

void Session::do_read_header() {
	std::cout << "req.header_length " << req.header_length << std::endl;
	auto self(shared_from_this());

	asio::async_read(socket_,
			asio::buffer(req.header, (std::size_t)req.header_length),
			[this, self](std::error_code ec, std::size_t) {
				//if (!ec && req.decode_header()) {
				if (!ec) {
					req.decode_message();
					std::cout << "body length : " << req.body_length << std::endl;
					std::cout << "header : " << req.header << std::endl;
					do_read_body();
				} else {
					std::cout << "error" << std::endl;
					std::cout << ec << std::endl;
				}
	});
}

void Session::do_read_body() {
	std::cout << "req.body_length " << req.body_length << std::endl;
	auto self(shared_from_this());
	asio::async_read(socket_,
			asio::buffer(req.body, (std::size_t)req.body_length),
			[this, self](std::error_code ec, std::size_t) {
				if (!ec) {
					std::cout << "body : " << req.body << std::endl;
					std::string lang="en";
					std::promise<std::string> promiseObj;
					std::future<std::string> futureObj = promiseObj.get_future();
					std::thread th(queryParser.execute, lang, std::string(req.body), &promiseObj);
					th.join();
					do_write(futureObj.get().c_str());
				} else {
					std::cout << "error" << std::endl;
					std::cout << ec << std::endl;
				}
	});
}

void Session::do_write(const char* response) {
	if (sizeof(response)==0 || response == NULL) {
		return;
	}
	std::cout << "response "<< std::endl;
	std::cout << response << std::endl;
	res.encode_message(const_cast<char*>(response));
	std::cout << "res.body_length " << res.body_length << std::endl;
	auto self(shared_from_this());
	asio::async_write(socket_,
			asio::buffer(res.body,
			res.body_length),
			[this, self](std::error_code ec, std::size_t /*length*/) {
				if (!ec) {
					std::cout << "body : " << res.body << std::endl;
				}
	});
}

