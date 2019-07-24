#include <memory>
#include <set>
#include <utility>
#include <iostream>
#include "asio.hpp"
#include "message.h"


class session : public std::enable_shared_from_this<session> {

	public:

		session(asio::ip::tcp::socket socket) : socket_(std::move(socket))
		{
		}

		void start() {
			do_read_header();
		}

		void deliver(const message& msg) {
			// do something here
		}

	private:

		asio::ip::tcp::socket socket_;
		std::size_t len = 0;
		char read_msg__[1023];

		void do_read_header() {
			auto self(shared_from_this());
//			asio::streambuf read_buffer;
//			auto bytes_transferred = asio::read(socket_, read_buffer);

			asio::async_read(socket_,
					asio::buffer(read_msg_.data(), message::header_length),
					[this, self](std::error_code ec, std::size_t) {
						if (!ec) {
							std::cout << "cormac " << std::endl;
							std::cout << "do read header " << read_msg_.data() << std::endl;
							do_read_body();
						} else {
							std::cout << "error" << std::endl;
							std::cout << ec << std::endl;
						}
			});
		}

		void do_read_body() {
			auto self(shared_from_this());
			asio::async_read(socket_,
					asio::buffer(read_msg_.body(), read_msg_.body_length()),
					[this, self](std::error_code ec, std::size_t /*length*/) {
					if (!ec) {
						std::cout << "do read body " + read_msg_.body_length() << std::endl;
	//					do_read_header();
					}
				});
		}
		message read_msg_;
};
