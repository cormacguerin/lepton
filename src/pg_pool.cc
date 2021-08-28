#include <iostream>
#include <thread>
#include <fstream>
#include <sstream>
#include <iostream>
#include "pg_pool.h"
#include "util.h"


PgPool::PgPool() {
}

PgPool::~PgPool() {
}

void PgPool::init(std::string database) {
    for ( auto i = 0; i< POOL_SIZE; ++i ) {
        m_pool.emplace ( std::make_shared<pqxx::connection>(createConnection(database)) );
    }
}

std::shared_ptr<pqxx::connection> PgPool::getConn() {
    std::unique_lock<std::mutex> lock_( m_mutex );

    while ( m_pool.empty() ){
        m_condition.wait( lock_ );
    }

    auto conn_ = m_pool.front();
    m_pool.pop();
    std::cout << "pg_pool.cc : num conns used " << POOL_SIZE -  m_pool.size() << std::endl;

    return conn_;
}

void PgPool::freeConn(std::shared_ptr<pqxx::connection> C)
{
    std::unique_lock<std::mutex> lock_( m_mutex );
    m_pool.push( C );
    lock_.unlock();
    m_condition.notify_one();
}

pqxx::connection PgPool::createConnection(std::string database) {
    std::cout << " database " << std::endl;
    std::cout << database << std::endl;


    auto config = getConfig();
    std::cout << " config.postgres_user " << std::endl;
    std::cout << config.postgres_user << std::endl;

    try {
      pqxx::connection C = pqxx::connection("dbname = " + database + " user = " + config.postgres_user + " password = " + config.postgres_password + " hostaddr = " + config.postgres_host + " port = " + config.postgres_port);
      if (C.is_open()) {
        std::cout << "Opened database successfully: " << C.dbname() << std::endl;
      } else {
        std::cout << "Can't open database" << std::endl;
      }
      return C;
    } catch(std::exception& e) {
      std::cout << "Exception in open database: " << e.what() << std::endl;
    }
}
