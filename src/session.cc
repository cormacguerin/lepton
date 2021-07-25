#include "session.h"
#include <set>
#include <utility>
#include <iostream>
#include <memory>
#include <future>


//Session::Session(asio::ip::tcp::socket socket, std::shared_ptr<IndexServer> indexServer) : socket_(std::move(socket)), is_(std::move(indexServer))
Session::Session(asio::ip::tcp::socket socket) : socket_(std::move(socket))
{
}

Session::~Session() 
{
}

request<char*> req;
response<std::string> res;

void Session::do_read_header() {

	auto self(shared_from_this());

	asio::async_read(socket_,
			asio::buffer(req.header, (std::size_t)req.header_length),
			[this, self](std::error_code ec, std::size_t) {
				if (!ec) {
					req.decode_message();
                    std::cout << "decode_message done" << std::endl;
					do_read_body();
				} else {
					std::cout << "session.cc do_read_header EC" << std::endl;
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
                    std::cout << "session.cc req.body - " << req.body << " - " << std::endl;
                    do_write(do_callback(req.body));
				} else {
					std::cout << "session.cc do_read_body EC" << std::endl;
					std::cout << ec << std::endl;
				}
				// free(req.body);
				std::cout << "session.cc : read_async done" << std::endl;
	});
}

void Session::do_write(std::string response) {
  /*
	std::cout << "session.cc : res.body_length " << res.body_length << std::endl;
	std::cout << "session.cc : responese.length " << response.length() << std::endl;
  std::cout << "session.cc - response - " << response << " - " << std::endl;
  */
	auto self(shared_from_this());
	asio::async_write(socket_,
			asio::buffer(response, response.length()),
			[this, self](std::error_code ec, std::size_t /*length*/) {
				if (!ec) {
				} else {
					std::cout << "session.cc : error - " << res.body << std::endl;
                    std::cout << ec << std::endl;
                }
	});
}

std::string Session::do_callback(std::string req) {
    std::cout << "session.cc - Callback(req) - " << req << " - " << std::endl;
    return Callback(req);
}

// set_callback is called from the invoking class
// in the case of query_server.cc this is bound to do_query
// in the case of management_sercer.cc it is bound do_management 
void Session::set_callback(std::function<std::string(std::string)> cb) {
    Callback = cb;
}

