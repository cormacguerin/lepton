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
	std::cout << "TEST" << std::endl;
}

Session::~Session() 
{
}

request<char*> req;
response<char*> res;

//void Session::start(IndexServer *indexServer) {
void Session::start(const std::shared_ptr<IndexServer> &indexServer) {
	/*
	std::cout << "  - - - - - - -  - - - - - - -  - - - - - - -  - - - - - - -  - - - - - - -  - - - - - - -  - - - - - - - " << std::endl;
					std::string lang="en";
					std::promise<std::string> promiseObj;
					std::future<std::string> futureObj = promiseObj.get_future();
					auto keepAlive = indexServer;
					(*indexServer).execute(lang, std::string("req.body"), std::move(promiseObj));
	*/
	is_ = indexServer;
	if (req.body == NULL) {
		std::cout << "req is null " << std::endl;
	} else {
		std::cout << "req is not null " << strlen(req.body) << std::endl;
	}
	free(req.body);
	free(res.body);
	do_read_header();
}

void Session::do_read_header() {
	std::cout << "req.header_length " << req.header_length << std::endl;
	auto self(shared_from_this());

	asio::async_read(socket_,
			asio::buffer(req.header, (std::size_t)req.header_length),
			[this, self](std::error_code ec, std::size_t) {
				if (!ec) {
					std::cout << "body length A : " << req.body_length << std::endl;
					std::cout << "header A : " << req.header << std::endl;
					req.decode_message();
					std::cout << "body length B : " << req.body_length << std::endl;
					std::cout << "header B : " << req.header << std::endl;
					do_read_body();
				} else {
					std::cout << "error" << std::endl;
					std::cout << ec << std::endl;
					return;
				}
	});
}

void Session::do_read_body() {

	std::cout << "req.body_length A " << req.body_length << std::endl;
	std::cout << "body A : " << req.body << std::endl;
	auto self(shared_from_this());
	asio::async_read(socket_,
			asio::buffer(req.body, (std::size_t)req.body_length),
			[this, self](std::error_code ec, std::size_t) {
				if (!ec) {
					std::cout << "req.body_length B " << req.body_length << std::endl;
					std::cout << "req.body : " << req.body << std::endl;
					std::string lang="en";
					std::promise<std::string> promiseObj;
					std::future<std::string> futureObj = promiseObj.get_future();
					is_.get()->execute(lang, std::string(req.body), std::move(promiseObj));
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

