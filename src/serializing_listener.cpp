#include "serializing_listener.h"

namespace serializing_listener{
    
void SerializingListener::OnTick(std::chrono::milliseconds time_delta
            , const model::Game& game
            , const players::Players& players
            , const extra_data::LostObjectsOnMaps& lost_objects_on_map){

    if(save_period_ == 0ms){
        return;
    }

    if(time_since_save_ + time_delta >= save_period_){
        std::filesystem::path temp_path = std::filesystem::weakly_canonical(snapshoot_path_.string() + "/../temp");
        std::ofstream temp_file(temp_path, std::ios::binary);
        serialization::MakeModelSerialize(temp_file, game, players, lost_objects_on_map);
        std::filesystem::rename(temp_path, snapshoot_path_);
        time_since_save_ = 0ms;
    }
    else{
        time_since_save_ += time_delta;
    }
}

}