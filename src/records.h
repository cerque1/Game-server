#pragma once
#include <string>
#include <pqxx/pqxx>

#include "tagged_uuid.h"

namespace domain{

namespace details{

    struct RecordTag{};

}

using RecordId = util::TaggedUUID<details::RecordTag>;

class Record{
public:
    Record(RecordId id, const std::string& dog_name, int score, int time)
        : id_(std::move(id)), dog_name_(dog_name), score_(score), time_(time){}

    const std::string& GetDogName() const{
        return dog_name_;
    }

    int GetScore() const{
        return score_;
    }

    int GetTime() const{
        return time_;
    }

    const RecordId& GetId() const{
        return id_;
    }

private:
    RecordId id_;
    std::string dog_name_;
    int score_;
    int time_;
};

class RecordRepository{ 
public:
    virtual void Save(const Record& record, pqxx::work& work) = 0;
    virtual void SaveRecords(const std::vector<Record>& records) = 0;
    virtual std::vector<Record> GetRecords(int start, int count) = 0;

protected:
    ~RecordRepository() = default;
};

}