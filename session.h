#include <asio.hpp>
#include "message.h"
#include "query_builder.h"
#include "index_server.h"


class Session : public std::enable_shared_from_this<Session> {

	public:

		Session(asio::ip::tcp::socket socket);
		~Session();
		void start(const std::shared_ptr<IndexServer> &indexServer);

	private:

		//IndexServer* is_;
		std::shared_ptr<IndexServer> is_;
		asio::ip::tcp::socket socket_;
		void do_read_header();
		void do_read_body();
		void do_write(std::string s);
//		request<char*> req;
//		response<char*> res;
		void run_query(std::promise<std::string> *promiseObj);

};
