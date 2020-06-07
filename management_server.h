#ifndef _MANAGEMENT_SERVER_H_
#define _MANAGEMENT_SERVER_H_

#include <pqxx/pqxx>
#include <asio.hpp>
#include "query_server.h"

class ManagementServer {
  private:
    pqxx::connection* C;
    void do_accept();
    const asio::ip::tcp::endpoint endpoint;
    asio::io_context io_context;
    asio::ip::tcp::acceptor acceptor_;
    std::string do_management(std::string body);
    std::vector<QueryServer*> servers;
    const std::vector<QueryServer*> &constServers = servers;
    void adminConnect();
    int port = 3334;
    std::string getStats();
    //  void startServerThread(int port, std::string database, std::string table);
    void startServerThread(int port, std::string database, std::string table);
    std::string toggleServing(std::string database, std::string table, std::string action);

  public:
    ManagementServer(short port);
    ~ManagementServer();
    void run();
    void runServer();
};
#endif

