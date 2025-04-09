#pragma once

#include <iostream>
#include <unordered_set>
#include <random>
#include <sstream>
#include <chrono>

#include "model.h"
#include "tagged.h"

namespace players{

using namespace std::literals;
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
    Player() = default;
    Player(const Player& other);
    Player(model::GameSession* game_session, std::shared_ptr<model::Dog> dog, Token token)
           : game_session_(game_session), dog_(dog), token_(std::move(token)){}

    int GetId() const{
        return dog_->GetId();
    }

    std::string GetMapId() const{
        return game_session_->GetMapId();
    }

    Token GetToken() const{
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

    bool IsRetired(int retired_time) const{
        return dog_->GetAfkTime() >= retired_time;
    }

private:
    model::GameSession* game_session_ = nullptr;
    std::shared_ptr<model::Dog> dog_ = nullptr;
    Token token_ = Token{""};
};

class Players{
public:
    Players() = default;

    Players(const Players& other);
    Players& operator=(const Players& other);

    void AddPlayer(std::string map_id, std::string token, int player_id, const players::Player& player);
    void SetNextId(int next_id){
        next_id_ = next_id;
    }

    Player* AddPlayer(std::string dog_name, model::GameSession* game_session);
    Player* FindByDogIdAndMapId(int dog_id, std::string map_id);
    Player* FindByToken(Token token);

    // возвращает id собак удаленных игроков
    std::unordered_map<std::string, std::vector<int>> EraseRetiredPlayers(int retires_time);

    const std::unordered_map<std::string, std::unordered_map<int, std::shared_ptr<Player>>>& GetPlayers() const;

private:
    std::unordered_map<std::string, std::unordered_map<int, std::shared_ptr<Player>>> players_;
    std::unordered_map<Token, std::shared_ptr<Player>, util::TaggedHasher<Token>> token_to_player;
    int next_id_ = 0;
    details::TokenGenerator token_generator_;
};

} // players