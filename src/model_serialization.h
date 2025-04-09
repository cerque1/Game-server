#include <boost/serialization/vector.hpp>
#include <boost/serialization/unordered_map.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include "model.h"
#include "player.h"
#include "extra_data.h"

namespace model {

template <typename Archive>
void serialize(Archive& ar, CollectedItem& item, [[maybe_unused]] const unsigned version) {
    ar&(item.id);
    ar&(item.type);
}

template <typename Archive>
void serialize(Archive& ar, Coordinates& coords, [[maybe_unused]] const unsigned version) {
    ar& coords.x;
    ar& coords.y;
}

template <typename Archive>
void serialize(Archive& ar, Speed& speed, [[maybe_unused]] const unsigned version) {
    ar& speed.horizontal;
    ar& speed.vertical;
}

}  // namespace model

namespace serialization {

// DogRepr (DogRepresentation) - сериализованное представление класса Dog
class DogRepr {
public:
    DogRepr() = default;

    explicit DogRepr(const model::Dog& dog)
        : id_(dog.GetId())
        , name_(dog.GetName())
        , pos_(dog.GetCoords())
        , speed_(dog.GetSpeed())
        , direction_(dog.GetDir())
        , score_(dog.GetScore())
        , bag_(dog.GetBag()) {
    }

    [[nodiscard]] model::Dog Restore() const {
        model::Dog dog{id_, name_, pos_};
        dog.SetSpeed(speed_);
        dog.SetDir(direction_);
        dog.AddScore(score_);
        for (const auto& item : bag_) {
            dog.AddCollectedItemToBag(item);
        }
        return dog;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar& id_;
        ar& name_;
        ar& pos_;
        ar& speed_;
        ar& direction_;
        ar& score_;
        ar& bag_;
    }

private:
    int id_ = 0;
    std::string name_;
    model::Coordinates pos_;
    model::Speed speed_;
    model::DirectionGeo direction_ = model::DirectionGeo::NORTH;
    int score_ = 0;
    std::vector<model::CollectedItem> bag_;
};

class GameSessionRepr{
public:
    GameSessionRepr() = default;

    explicit GameSessionRepr(const model::GameSession& game_session) 
        : map_id_(game_session.GetMapId()){
        for(auto [id, dog] : game_session.GetDogs()){
            dogs_[id] = DogRepr{*dog};
        }
    }

    [[nodiscard]] model::GameSession Restore(const model::Game& game, bool is_random_generate) const{
        model::GameSession game_session{game.FindMap(model::Map::Id(map_id_)), is_random_generate};
        for(auto [id, dog] : dogs_){
            game_session.AddDog(id, dog.Restore());
        }
        return game_session;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version){
        ar& map_id_;
        ar& dogs_;
    }

private:
    std::string map_id_;
    std::unordered_map<std::uint64_t, DogRepr> dogs_;
};

class PlayerRepr{
public:

    PlayerRepr() = default;

    explicit PlayerRepr(const players::Player& player) 
        : map_id_(player.GetMapId())
        , dog_id_(player.GetId())
        , token_(*player.GetToken()){
    }

    [[nodiscard]] players::Player Restore(const model::Game& game){
        model::GameSession* game_session = game.FindGameSessionFromMapId(model::Map::Id{map_id_});
        players::Player player{game_session, game_session->GetDogs().at(dog_id_), players::Token{token_}};
        return player;
    }

    std::string GetToken() const{
        return token_;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version){
        ar& map_id_;
        ar& dog_id_;
        ar& token_;
    }

private:
    std::string map_id_;
    int dog_id_;
    std::string token_;
};

class PlayersRepr{
public:

    PlayersRepr() = default;

    explicit PlayersRepr(const players::Players& players) {
        for(auto [map_id, players_on_map] : players.GetPlayers()){
            for(auto [player_id, player] : players_on_map){
                players_[map_id][player_id] = PlayerRepr{*player};
                ++next_id;
            }
        }
    }

    [[nodiscard]] players::Players Restore(const model::Game& game){
        players::Players players;

        for(auto [map_id, players_on_map] : players_){
            for(auto [player_id, player] : players_on_map){
                players.AddPlayer(map_id, player.GetToken(), player_id, player.Restore(game));
            }
        }

        players.SetNextId(next_id);

        return players;
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version){
        ar& players_;
        ar& next_id;
    }

private:
    std::unordered_map<std::string, std::unordered_map<int, PlayerRepr>> players_;
    int next_id = 0;
};

class LostObjectRepr{
public:
    LostObjectRepr() = default;

    explicit LostObjectRepr(const extra_data::LostObject& lost_obj)
        : id_(lost_obj.id)
        , type_(lost_obj.type)
        , coords_(lost_obj.coords){
    }

    [[nodiscard]] extra_data::LostObject Restore(){
        return extra_data::LostObject{id_, type_, coords_};
    }

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version){
        ar& id_;
        ar& type_;
        ar& coords_;
    }

private:
    unsigned id_;
    unsigned type_;
    model::Coordinates coords_;
};

template <typename Output>
Output& MakeModelSerialize(Output& output
                , const model::Game& game
                , const players::Players& players
                , const extra_data::LostObjectsOnMaps& lost_objects_on_map){
    boost::archive::binary_oarchive ar{output};

    std::vector<serialization::GameSessionRepr> game_sessions_repr;
    for(auto game_session : game.GetGameSession()){
        game_sessions_repr.push_back(serialization::GameSessionRepr{*game_session});
    }

    serialization::PlayersRepr players_repr{players};
    std::unordered_map<std::string, std::vector<extra_data::LostObject>> lost_objects = lost_objects_on_map.GetLostObjectsOnMaps();
    std::unordered_map<std::string, std::vector<serialization::LostObjectRepr>> lost_objects_repr;
    for(auto [map_id, lost_obj] : lost_objects){
        lost_objects_repr[map_id] = {};
        for(auto object : lost_obj){
            lost_objects_repr[map_id].push_back(LostObjectRepr{object});
        }
    }

    ar << game_sessions_repr;
    ar << players_repr;
    ar << lost_objects_repr;

    return output;
}

}  // namespace serialization
