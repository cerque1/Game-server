#include "player.h"

#include <iomanip>
#include <random>

namespace players{

namespace details{

    Token TokenGenerator::GenerateToken(){
        std::uint64_t num1 = generator1_();
        std::uint64_t num2 = generator2_();

        std::stringstream ss1, ss2;
        ss1 << std::setfill('0') << std::hex << std::setw(16) << num1 << std::setw(16) << num2;
        return Token{ss1.str()};
    }
    
}

Player::Player(const Player& other) 
        : game_session_(other.game_session_)
        , dog_(other.dog_)
        , token_(other.token_){
}

Players::Players(const Players& other)
        : players_(other.players_)
        , token_to_player(other.token_to_player)
        , next_id_(other.next_id_){
}

Players& Players::operator=(const Players& other){
    players_ = other.players_;
    token_to_player = other.token_to_player;
    next_id_ = other.next_id_;
    return *this;
}

void Players::AddPlayer(std::string map_id, std::string token, int player_id, const players::Player& player){
    std::shared_ptr<players::Player> player_ptr = std::make_shared<players::Player>(player);
    players_[map_id][player_id] = player_ptr;
    token_to_player[players::Token{token}] = player_ptr;
}

Player* Players::AddPlayer(std::string dog_name, model::GameSession* game_session){
    Token token = token_generator_.GenerateToken();
    std::shared_ptr<Player> player = std::make_shared<Player>(game_session, game_session->AddDog(dog_name, next_id_), token);
    token_to_player[token] = player;
    players_[game_session->GetMapId()][next_id_++] = player;
    return &*player;
}

Player* Players::FindByDogIdAndMapId(int dog_id, std::string map_id){
    if(players_.find(map_id) != players_.end()){
        if(players_[map_id].find(dog_id) != players_[map_id].end()){
            return &*players_[map_id][dog_id];
        }
    }
    return nullptr;
}

Player* Players::FindByToken(Token token){
    if(token_to_player.find(token) != token_to_player.end()){
        return &*token_to_player[token];
    }
    return nullptr;
}

std::unordered_map<std::string, std::vector<int>> Players::EraseRetiredPlayers(int retires_time){
    std::unordered_map<std::string, std::vector<int>> retired_dogs_id;

    for(auto [map_id, players_on_map] : players_){
        for(auto [player_id, player] : players_on_map){
            if(player->IsRetired(retires_time)){
                retired_dogs_id[map_id].push_back(player->GetId());
                token_to_player.erase(player->GetToken());
            }
        }
    }

    for(auto [map_id, erased_players] : retired_dogs_id){
        for(auto player_id : erased_players){
            players_[map_id].erase(player_id);
        }
    }

    return retired_dogs_id;
}

const std::unordered_map<std::string, std::unordered_map<int, std::shared_ptr<Player>>>& Players::GetPlayers() const{
    return players_;
}

}