#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <map>
#include <optional>
#include <memory>

#include "tagged.h"
#include "collision_detector.h"
#include "records.h"

namespace model {

using namespace std::literals;

struct KeyLiterals{
    inline static const std::string x = "x";
    inline static const std::string y = "y";
    inline static const std::string w = "w";
    inline static const std::string h = "h";
};

using Dimension = int;
using Coord = Dimension;

struct Point {
    Coord x, y;
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

struct Coordinates{
    double x = 0;
    double y = 0;

    Coordinates() = default;
    Coordinates(double _x, double _y) : x(_x), y(_y) {}
};

bool operator==(const Coordinates& lhs, const Coordinates& rhs);

struct Speed{
    double horizontal = 0;
    double vertical = 0;
};

bool operator==(const Speed& lhs, const Speed& rhs);

struct MoveInfo{
    Coordinates start;
    Coordinates end;
};

struct CollectedItem{
    unsigned id;
    unsigned type;
};

struct RecordInfo{
    std::string name;
    int score;
    double playing_time;
};

enum DirectionGeo{
    NORTH = 0,
    SOUTH,
    WEST,
    EAST,
    NONE
};

class Road {
    struct HorizontalTag {
        HorizontalTag() = default;
    };

    struct VerticalTag {
        VerticalTag() = default;
    };

public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road() = default;
    Road(HorizontalTag, Point start, Coord end_x) noexcept
        : start_{start}
        , end_{end_x, start.y} {
    }

    Road(VerticalTag, Point start, Coord end_y) noexcept
        : start_{start}
        , end_{start.x, end_y} {
    }

    bool IsHorizontal() const noexcept {
        return start_.y == end_.y;
    }

    bool IsVertical() const noexcept {
        return start_.x == end_.x;
    }

    Point GetStart() const noexcept {
        return start_;
    }

    Point GetEnd() const noexcept {
        return end_;
    }

private:
    Point start_;
    Point end_;
};

class Building {
public:
    explicit Building(Rectangle bounds) noexcept
        : bounds_{bounds} {
    }

    const Rectangle& GetBounds() const noexcept {
        return bounds_;
    }

private:
    Rectangle bounds_;
};

class Office {
public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, Point position, Offset offset) noexcept
        : id_{std::move(id)}
        , position_{position}
        , offset_{offset} {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    Point GetPosition() const noexcept {
        return position_;
    }

    Offset GetOffset() const noexcept {
        return offset_;
    }

private:
    Id id_;
    Point position_;
    Offset offset_;
};

class Map {
public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;

    Map(Id id, std::string name, double dog_speed, int bag_capacity) noexcept
        : id_(std::move(id))
        , name_(std::move(name))
        , dog_speed_(dog_speed)
        , bag_capacity_(bag_capacity) {
    }

    const Id& GetId() const noexcept {
        return id_;
    }

    const std::string& GetName() const noexcept {
        return name_;
    }

    const Buildings& GetBuildings() const noexcept {
        return buildings_;
    }

    const Roads& GetRoads() const noexcept {
        return roads_;
    }

    const Offices& GetOffices() const noexcept {
        return offices_;
    }

    double GetDogSpeed() const noexcept{
        return dog_speed_;
    }

    int GetBagCapacity() const noexcept{
        return bag_capacity_;
    }

    void AddRoad(const Road& road) {
        roads_.emplace_back(road);
    }

    void AddBuilding(const Building& building) {
        buildings_.emplace_back(building);
    }

    void AddOffice(Office office);

    Coordinates CanGoToPoint(const Coordinates& A, const Coordinates& B, model::DirectionGeo dir) const;

private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    bool IsContainsInRoute(const Road& road, const Coordinates& point) const;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;

    double dog_speed_;
    int bag_capacity_;

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
};

class Dog{ 
public:
    Dog() = default;

    Dog(int id, std::string name, Coordinates coords) : id_(id), name_(name), coords_(coords){
        dir_ = DirectionGeo::NORTH;
    }

    int GetId() const{
        return id_;
    }
    std::string GetName() const{
        return name_;
    }

    Coordinates GetCoords() const{
        return coords_;
    }

    Speed GetSpeed() const{
        return speed_;
    }
    
    DirectionGeo GetDir() const{
        return dir_;
    }

    void SetSpeed(const Speed& new_speed){
        speed_ = new_speed;
    }

    void SetCoords(const Coordinates& coords){
        coords_ = coords;
    }

    void SetDir(const DirectionGeo& dir){
        dir_ = dir;
        if(dir != DirectionGeo::NONE){
            is_change_direction_in_tick_ = true;
        }
    }

    void AddCollectedItemToBag(CollectedItem collected_item){
        bag_.push_back(std::move(collected_item));
    }

    size_t GetBagSize() const{
        return bag_.size();
    }

    const std::vector<CollectedItem>& GetBag() const{
        return bag_;
    }

    int GetScore() const{
        return score_;
    }

    void AddScore(int value){
        score_ += value;
    }

    void ClearBag(){
        bag_.clear();
    }

    int GetAfkTime() const{
        return afk_time_;
    }

    void AddAfkTime(int afk_time){
        afk_time_ += afk_time;
    }

    void ClearAfkTime() {
        afk_time_ = 0;
    }

    int GetPlayTime() const{
        return play_time_;
    }

    void AddPlayTime(int play_time){
        play_time_ += play_time;
    }

    bool GetChangeDirInTick() const{
        return is_change_direction_in_tick_;
    }

    void SetChangeDirInTick(bool change_dir_in_tick){
        is_change_direction_in_tick_ = change_dir_in_tick;
    }

private:
    int id_;
    std::string name_;
    Coordinates coords_;
    Speed speed_;
    DirectionGeo dir_;
    std::vector<CollectedItem> bag_;
    int score_ = 0;
    int afk_time_ = 0;
    int play_time_ = 0;
    bool is_change_direction_in_tick_ = false;
};

namespace generate_coords{

double GenerateRandomDouble(int start, int end);
Coordinates GenerateRandomPointOnMap(const Map& map);

} // generate_coords

class GameSession{
public:
    using PlayerInfo = std::vector<std::pair<int, std::string>>;

    GameSession(const Map* map, bool is_random_generate) : map_(map), is_random_generate_(is_random_generate){}

    std::shared_ptr<Dog> AddDog(std::string name, int id);
    
    void AddDog(uint64_t id, const Dog& dog);

    std::string GetMapId() const{
        return *(map_->GetId());
    }

    double GetMapDogSpeed() const{
        return map_->GetDogSpeed();
    }
    
    std::vector<std::pair<int, std::string>> GetPlayersInfo() const;

    const std::unordered_map<std::uint64_t, std::shared_ptr<Dog>>& GetDogs() const{
        return dogs_;
    }

    int GetDogsCount() const {
        return dogs_.size();
    }

    bool GetIsRandomGenerate() const { 
        return is_random_generate_;
    }

    const Map* GetMap() const {
        return map_;
    }

    std::unordered_map<int, MoveInfo> MakeActionsAtTime(int time);

    void EraseDogsById(std::vector<int> dogs_id, std::vector<domain::Record>& records);
    
private:
    bool is_random_generate_;
    const Map* map_;
    std::unordered_map<std::uint64_t, std::shared_ptr<Dog>> dogs_;
};

using MapIdToMovesInfo = std::unordered_map<Map::Id, std::unordered_map<int, MoveInfo>, util::TaggedHasher<Map::Id>>;

class Game {
public:
    using Maps = std::vector<Map>;
    Game() = default;

    void AddMap(Map map);

    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    const Map* FindMap(const Map::Id& id) const noexcept;

    void AddGameSession(GameSession&& game_sessoion);
    model::GameSession* AddGameSession(std::string map_id);

    model::GameSession* FindGameSessionFromMapId(const Map::Id& id) const;

    const std::vector<std::shared_ptr<GameSession>>& GetGameSession() const{
        return game_sessions_;
    }

    MapIdToMovesInfo MakeActionsAtTime(int time = 1);

    void SetRandomGenerate(bool is_random_generate){
        is_random_generate_ = is_random_generate;
    }

    // возвращает результаты удаленных игроков
    std::vector<domain::Record> EraseRetiredDogs(std::unordered_map<std::string, std::vector<int>> dogs_id);

private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;
    using GameSessionMapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    Map* FindMapForGameSession(const Map::Id& id);

    bool is_random_generate_;
    Maps maps_;
    std::vector<std::shared_ptr<model::GameSession>> game_sessions_;
    MapIdToIndex map_id_to_index_;
    GameSessionMapIdToIndex game_session_to_map_id_;
};

}  // namespace model
