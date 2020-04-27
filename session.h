#include <asio.hpp>
#include "message.h"
#include "query_builder.h"
#include "index_server.h"


class Session : public std::enable_shared_from_this<Session> {

	public:

		Session(asio::ip::tcp::socket socket);
		~Session();
//		void start(const std::shared_ptr<IndexServer> &indexServer);
//    std::string (*callback)(std::string);
    std::function<std::string(std::string)> Callback;
		void do_read_header();
		void do_read_body();
		void do_write(std::string s);
    // void set_callback(std::function<void(std::string)>);
    void set_callback(std::function<std::string(std::string)>);
    // void set_callback(std::string(*cb)(std::string));
    std::string do_callback(std::string req);

	private:

//		std::shared_ptr<IndexServer> is_;
		asio::ip::tcp::socket socket_;
//		request<char*> req;
//		response<char*> res;
//		void run_query(std::promise<std::string> *promiseObj);

};
