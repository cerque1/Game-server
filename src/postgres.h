#pragma once

#include "records.h"
#include "connection_pool.h"

#include <pqxx/pqxx>
#include <pqxx/connection>
#include <pqxx/transaction>
#include <pqxx/zview.hxx>

namespace postgres{

class RecordRepositoryImpl : public domain::RecordRepository{
public:
    RecordRepositoryImpl(pqxx::connection& connection) : connection_(connection){}

    void Save(const domain::Record& record, pqxx::work& work) override;
    void SaveRecords(const std::vector<domain::Record>& records) override;
    std::vector<domain::Record> GetRecords(int start, int count) override;

private:
    pqxx::connection& connection_;
};

class Database{
public:
    explicit Database(size_t num_threads, const char* db_url);

    std::shared_ptr<RecordRepositoryImpl> GetRecordRepo() &{
        auto conn = connection_pool_.GetConnection();
        std::shared_ptr<RecordRepositoryImpl> record_repr = std::make_shared<RecordRepositoryImpl>(*conn);
        return record_repr;
    }

private:
    ConnectionPool connection_pool_;
};

} //postgres