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

		void do_read_header() {
			std::cout << "do read header A" << std::endl;
			auto self(shared_from_this());
			asio::async_read(socket_,
					asio::buffer(read_msg_.data(), message::header_length),
					[this, self](std::error_code ec, std::size_t /*length*/) {
						if (!ec && read_msg_.decode_header()) {
							std::cout << "do read header B " << std::endl;
							do_read_body();
						} else {
					}
				});
		}

		void do_read_body() {
			std::cout << "do read body A " << std::endl;
			auto self(shared_from_this());
			asio::async_read(socket_,
					asio::buffer(read_msg_.body(), read_msg_.body_length()),
					[this, self](std::error_code ec, std::size_t /*length*/) {
					if (!ec) {
						std::cout << "do read body B " << std::endl;
						do_read_header();
					}
				});
		}
		message read_msg_;
};
