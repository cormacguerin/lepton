#include "asio.hpp"
#include "message.h"
#include "query_parser.h"


class Session : public std::enable_shared_from_this<Session> {

	public:

		Session(asio::ip::tcp::socket socket);
		~Session();

		void start();

	private:

		asio::ip::tcp::socket socket_;
		void do_read_header();
		void do_read_body();
		void do_write(const char*);
		request<char*> req;
		response<char*> res;
		QueryParser queryParser;
		void run_query(std::promise<std::string> *promiseObj);

};
