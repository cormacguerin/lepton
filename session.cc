#include "session.h"
#include "message.h"
#include <memory>
#include <set>
#include <utility>
#include <iostream>


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

			request<char[1024]> req;
			std::size_t len = 0;
			char read_msg_[1023];

		void Session::do_read_header() {
			std::size_t len = 14;
			auto self(shared_from_this());
//			asio::streambuf read_buffer;
//			auto bytes_transferred = asio::read(socket_, read_buffer);

			std::cout << "req.header_length " << req.header_length << std::endl;

			asio::async_read(socket_,
					asio::buffer(req.body, (std::size_t)req.header_length),
					//asio::buffer(req.body, len),
					[this, self](std::error_code ec, std::size_t) {
						if (!ec) {
							std::cout << "header : " << req.body << std::endl;
						//	do_read_body();
						} else {
							std::cout << "error" << std::endl;
							std::cout << ec << std::endl;
						}
			});
		}

		/*
		void do_read_body() {
			auto self(shared_from_this());
			asio::async_read(socket_,
					asio::buffer(read_msg_.body(), read_msg_.body_length()),
					[this, self](std::error_code ec, std::size_t) {
					if (!ec) {
						std::cout << "do read body " + read_msg_.body_length() << std::endl;
	//					do_read_header();
					}
				});
		}
		*/
		void Session::test() {
			const char* x = "test";
			//static const bool x = false;
			//using request = read_msg_(){header_length=10};

			request<std::string> req;
			req.body = "soemthing";
		}

