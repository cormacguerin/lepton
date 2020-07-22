#include <asio.hpp>
#include "message.h"
#include "query_builder.h"
#include "index_server.h"


class Session : public std::enable_shared_from_this<Session> {

	public:

		Session(asio::ip::tcp::socket socket);
		~Session();
        std::function<std::string(std::string)> Callback;
		void do_read_header();
		void do_read_body();
		void do_write(std::string s);
        void set_callback(std::function<std::string(std::string)>);
        std::string do_callback(std::string req);

	private:

		asio::ip::tcp::socket socket_;

};
