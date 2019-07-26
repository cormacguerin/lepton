#include "message.h"
#include <memory>
#include <set>
#include <utility>
#include <iostream>
#include "asio.hpp"


class session : public std::enable_shared_from_this<session> {

	public:

		session(asio::ip::tcp::socket socket) : socket_(std::move(socket))
		{
		}

		void start() {
			do_read_header();
		}

		/*
		void deliver(const message& msg) {
			// do something here
		}
		*/

	private:

		asio::ip::tcp::socket socket_;
		std::size_t len = 0;
		char read_msg_[1023];

		void do_read_header() {
			auto self(shared_from_this());
//			asio::streambuf read_buffer;
//			auto bytes_transferred = asio::read(socket_, read_buffer);

			asio::async_read(socket_,
					asio::buffer(read_msg_, message<true,std::string>::header_length),
					[this, self](std::error_code ec, std::size_t) {
						if (!ec) {
							std::cout << "cormac " << std::endl;
							std::cout << "do read header " << read_msg_ << std::endl;
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
		const char* x = "test";
		//static const bool x = false;
		//using request = read_msg_(){header_length=10};

		request<std::string> req;
		req.body = "this is a request";

};
