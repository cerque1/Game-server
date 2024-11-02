#pragma once

#include <iostream>
#include <unordered_set>
#include <random>
#include <sstream>

#include "model.h"
#include "tagged.h"

namespace players{

struct TokenTag{
};

using Token = util::Tagged<std::string, TokenTag>;

namespace details{

    class TokenGenerator{
    public:
        Token GenerateToken();
    private:
        std::random_device random_device_;
        std::mt19937_64 generator1_{[this] {
            std::uniform_int_distribution<std::mt19937_64::result_type> dist;
            return dist(random_device_);
        }()};
        std::mt19937_64 generator2_{[this] {
            std::uniform_int_distribution<std::mt19937_64::result_type> dist;
            return dist(random_device_);
        }()};
    };

} // details

class Player{
public:
    Player(model::GameSession* game_session, std::shared_ptr<model::Dog> dog, Token token)
           : game_session_(game_session), dog_(dog), token_(std::move(token)){}

    int GetId(){
        return dog_->GetId();
    }

    std::string GetMapId(){
        return game_session_->GetMapId();
    }

    Token GetToken(){
        return token_;
    }

    void SetDogSpeed(const model::Speed& new_speed){
        dog_->SetSpeed(new_speed);
    }

    void SetDogDir(const model::DirectionGeo& dir){
        dog_->SetDir(dir);
    }

    const model::GameSession* GetGameSession() const{
        return game_session_;
    }

private:
    model::GameSession* game_session_;
    std::shared_ptr<model::Dog> dog_;
    Token token_;
};

class Players{
public:
    Player* AddPlayer(std::string dog_name, model::GameSession* game_session);
    Player* FindByDogIdAndMapId(int dog_id, std::string map_id);
    Player* FindByToken(Token token);
private:
    std::unordered_map<std::string, std::unordered_map<int, std::shared_ptr<Player>>> players_;
    std::unordered_map<Token, std::shared_ptr<Player>, util::TaggedHasher<Token>> token_to_player;
    int next_id = 0;
    details::TokenGenerator token_generator_;
};

} // players