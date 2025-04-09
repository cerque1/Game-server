#include "postgres.h"

namespace postgres{

void RecordRepositoryImpl::Save(const domain::Record& record, pqxx::work& work){
    //доработать
    work.exec_params(R"(
        INSERT INTO retired_players VALUES($1, $2, $3, $4);
    )", record.GetId().ToString(), record.GetDogName(), record.GetScore(), record.GetTime());
}

void RecordRepositoryImpl::SaveRecords(const std::vector<domain::Record>& records) {
    pqxx::work work{connection_};

    for(auto record : records){
        Save(record, work);
    }

    work.commit();
}

std::vector<domain::Record> RecordRepositoryImpl::GetRecords(int start, int count){
    pqxx::work work{connection_};
    std::vector<domain::Record> records;

    for(auto record : work.exec_params(R"(
            SELECT * FROM retired_players ORDER BY score DESC, play_time, name LIMIT $1 OFFSET $2;
        )", count, start)){
        records.emplace_back(domain::RecordId::FromString(record[0].as<std::string>())
                            , record[1].as<std::string>()
                            , record[2].as<int>()
                            , record[3].as<double>());
    }

    return records;
}

Database::Database(size_t num_threads, const char* db_url) 
    : connection_pool_(num_threads, [db_url]{
            return std::make_shared<pqxx::connection>(db_url);
        }){
    auto conn = connection_pool_.GetConnection();
    pqxx::work work{*conn};

    work.exec_params(R"(
        CREATE TABLE IF NOT EXISTS retired_players (
            id UUID PRIMARY KEY,
            name varchar(100) NOT NULL,
            score int NOT NULL,
            play_time int NOT NULL
        );
    )");

    work.exec_params(R"(
        CREATE INDEX IF NOT EXISTS retired_players_idx ON retired_players (score DESC, play_time, name);
    )");

    work.commit();
}

} // postgres