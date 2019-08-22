#include "asio.hpp"
#include "message.h"
#include "query_builder.h"
#include "index_server.h"


class Session : public std::enable_shared_from_this<Session> {

	public:

		Session(asio::ip::tcp::socket socket, std::unique_ptr<IndexServer> indexServer);
		~Session();

		void start();

	private:

		std::unique_ptr<IndexServer> is_;
		asio::ip::tcp::socket socket_;
		void do_read_header();
		void do_read_body();
		void do_write(const char*);
		request<char*> req;
		response<char*> res;
		QueryBuilder queryBuilder;
		void run_query(std::promise<std::string> *promiseObj);

};
