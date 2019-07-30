#include "asio.hpp"
#include "message.h"


class Session : public std::enable_shared_from_this<Session> {

	public:

		Session(asio::ip::tcp::socket socket);
		~Session();

		void start();

	private:

		asio::ip::tcp::socket socket_;
		void do_read_header();
		void do_read_body();

};
