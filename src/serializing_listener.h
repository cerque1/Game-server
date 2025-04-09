#include <chrono>
#include <fstream>
#include <filesystem>

#include "model_serialization.h"

namespace serializing_listener{

using namespace std::chrono_literals;

class ApplicationListener{
public:
    virtual void OnTick(std::chrono::milliseconds time_delta
                , const model::Game& game
                , const players::Players& players
                , const extra_data::LostObjectsOnMaps& lost_objects_on_map) = 0;
};

class SerializingListener : public ApplicationListener{
public:
    SerializingListener(std::chrono::milliseconds save_period, const std::filesystem::path& snapshoot_path)
        : save_period_(save_period), snapshoot_path_(snapshoot_path){}

    void OnTick(std::chrono::milliseconds time_delta
                , const model::Game& game
                , const players::Players& players
                , const extra_data::LostObjectsOnMaps& lost_objects_on_map) override;

    const std::filesystem::path& GetPath() const{
        return snapshoot_path_;
    }

private:
    std::chrono::milliseconds time_since_save_ = 0ms;
    std::chrono::milliseconds save_period_;
    std::filesystem::path snapshoot_path_;
};

} //serializing_listener