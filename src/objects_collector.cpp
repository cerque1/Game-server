#include "objects_collector.h"
#include <set>

namespace objects_collector{

struct Events{
    std::vector<GatheringEvent> items_events;
    std::vector<GatheringEvent> offices_events;
};

Events GetEventsOnMap(const std::vector<model::Office>& offices_on_map, 
                                           const std::vector<extra_data::LostObject>& lost_objects, 
                                           const std::unordered_map<int, model::MoveInfo>& moves_info){
    std::vector<Gatherer> players;
    for(auto [id, move_info] : moves_info){
        players.push_back({geom::Point2D(move_info.start.x, move_info.start.y), geom::Point2D(move_info.end.x, move_info.end.y), 0.6});
    }

    std::vector<Item> items;
    for(auto lost_obj : lost_objects){
        items.push_back({geom::Point2D(lost_obj.coords.x, lost_obj.coords.y), 0});
    }

    std::vector<Item> offices;
    for(auto office : offices_on_map){
        offices.push_back(Item(geom::Point2D(office.GetPosition().x, office.GetPosition().y), 0.5));
    }

    ItemPlayersProvider items_players_provider{items, players};
    ItemPlayersProvider offices_players_provider{offices, players};

    return {FindGatherEvents(items_players_provider),
            FindGatherEvents(offices_players_provider)};
}

void CollectObjects(model::Game& game, extra_data::LostObjectsOnMaps& lost_objects_on_maps, const model::MapIdToMovesInfo& map_id_to_moves){
    for(auto [map_id, moves_info] : map_id_to_moves){
        Events events_on_map = GetEventsOnMap(game.FindMap(map_id)->GetOffices(), lost_objects_on_maps.GetLostObjects(*map_id), moves_info);

        auto dogs = game.FindGameSessionFromMapId(map_id)->GetDogs();
        const std::vector<extra_data::LostObject>& lost_objects = lost_objects_on_maps.GetLostObjects(*map_id);

        // для проверки был ли данный предмет поднят ранее
        std::set<int> collected_items;

        for(auto item_event_index = 0, office_event_index = 0; item_event_index < events_on_map.items_events.size() || office_event_index < events_on_map.offices_events.size();){
            if(item_event_index < events_on_map.items_events.size()){
                if(office_event_index < events_on_map.offices_events.size()){
                    if(events_on_map.items_events[item_event_index].time > events_on_map.offices_events[office_event_index].time){
                        GatheringEvent base_event = events_on_map.offices_events[office_event_index];
                        auto dog = std::next(dogs.begin(), base_event.gatherer_id)->second;

                        for(auto items_in_bag : dog->GetBag()){
                            dog->AddScore(lost_objects_on_maps.GetObjectValue(*map_id, items_in_bag.type));
                        }
                        dog->ClearBag();

                        ++office_event_index;
                        continue;
                    }
                }

                GatheringEvent collect_event = events_on_map.items_events[item_event_index];

                if(!collected_items.contains(collect_event.item_id) 
                    && std::next(dogs.begin(), collect_event.gatherer_id)->second->GetBagSize() < game.FindMap(map_id)->GetBagCapacity())
                {
                    std::next(dogs.begin(), collect_event.gatherer_id)->second->AddCollectedItemToBag(
                                                                    {lost_objects.at(collect_event.item_id).id, lost_objects.at(collect_event.item_id).type});
                    collected_items.insert(collect_event.item_id);
                }
                ++item_event_index;
            }
            else if(office_event_index < events_on_map.offices_events.size()){
                GatheringEvent base_event = events_on_map.offices_events[office_event_index];
                auto dog = std::next(dogs.begin(), base_event.gatherer_id)->second;

                for(auto items_in_bag : dog->GetBag()){
                    dog->AddScore(lost_objects_on_maps.GetObjectValue(*map_id, items_in_bag.type));
                }
                dog->ClearBag();

                ++office_event_index;
                continue;   
            }
        }

        lost_objects_on_maps.EraseLostObjectsOnMap(*map_id, collected_items.begin(), collected_items.end());
    }
}

};