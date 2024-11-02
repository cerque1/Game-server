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

Player* Players::AddPlayer(std::string dog_name, model::GameSession* game_session){
    Token token = token_generator_.GenerateToken();
    std::shared_ptr<Player> player = std::make_shared<Player>(game_session, game_session->AddDog(dog_name, next_id), token);
    token_to_player[token] = player;
    players_[game_session->GetMapId()][++next_id] = player;
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

}