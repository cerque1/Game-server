#pragma once

#include "extra_data.h"
#include "model.h"

namespace objects_collector{

namespace {
    
    using namespace collision_detector;

    class ItemPlayersProvider : public ItemGathererProvider{
    public:

        ItemPlayersProvider(std::vector<Item> items, std::vector<Gatherer> players) 
                : items_count_(items.size()), items_(items), 
                  players_count_(players.size()), players_(players) {}

        size_t ItemsCount() const override{
            return items_count_;
        }

        Item GetItem(size_t idx) const override{
            return items_.at(idx);
        }

        size_t GatherersCount() const override{
            return players_count_;
        }

        Gatherer GetGatherer(size_t idx) const override{
            return players_.at(idx);
        }

    private:
        size_t items_count_;
        std::vector<collision_detector::Item> items_;
        size_t players_count_;
        std::vector<collision_detector::Gatherer> players_;
    };

}

void CollectObjects(model::Game& game, extra_data::LostObjectsOnMaps& lost_objects_on_maps, const model::MapIdToMovesInfo& map_id_to_moves);

};