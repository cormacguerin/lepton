#ifndef _PG_POOL_H_
#define _PG_POOL_H_

#include <mutex>
#include <queue>
#include <condition_variable>
#include <string>
#include <pqxx/pqxx>

class PgPool {

public:

    PgPool();
    ~PgPool();
    std::shared_ptr<pqxx::connection> getConn();
    void freeConn(std::shared_ptr<pqxx::connection> C);
    void init(std::string database);

private:

    const int POOL_SIZE = 5;
    std::string db;

    pqxx::connection createConnection(std::string database);
    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::queue<std::shared_ptr<pqxx::connection>> m_pool;

};

#endif
