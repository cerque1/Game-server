#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <map>
#include <memory>

#include "tagged.h"

namespace model {

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
};

bool operator==(const Coordinates& lhs, const Coordinates& rhs);

struct Speed{
    double horizontal = 0;
    double vertical = 0;
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

    Map(Id id, std::string name, double dog_speed) noexcept
        : id_(std::move(id))
        , name_(std::move(name))
        , dog_speed_(dog_speed) {
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

    OfficeIdToIndex warehouse_id_to_index_;
    Offices offices_;
};

class Dog{ 
public:
    Dog(int id, std::string name, Coordinates coords) : id_(id), name_(name), coords_(coords){
        dir_ = DirectionGeo::NORTH;
    }

    int GetId(){
        return id_;
    }
    std::string GetName(){
        return name_;
    }

    Coordinates GetCoords(){
        return coords_;
    }

    Speed GetSpeed(){
        return speed_;
    }

    void SetSpeed(const Speed& new_speed){
        speed_ = new_speed;
    }

    void SetCoords(const Coordinates& coords){
        coords_ = coords;
    }

    void SetDir(const DirectionGeo& dir){
        dir_ = dir;
    }

    DirectionGeo GetDir(){
        return dir_;
    }

private:
    int id_;
    std::string name_;
    Coordinates coords_;
    Speed speed_;
    DirectionGeo dir_;
};

namespace generate_coords{

double GenerateRandomDouble(int start, int end);
Coordinates GenerateRandomPointOnMap(const Map* map);

} // generate_coords

class GameSession{
public:
    using PlayerInfo = std::vector<std::pair<int, std::string>>;

    GameSession(const Map* map, bool is_random_generate) : map_(map), is_random_generate_(is_random_generate){}

    std::shared_ptr<Dog> AddDog(std::string name, int id);

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

    void MakeActionsAtTime(int time);
    
private:
    bool is_random_generate_;
    const Map* map_;
    std::unordered_map<std::uint64_t, std::shared_ptr<Dog>> dogs_;
};

class Game {
public:
    using Maps = std::vector<Map>;

    void AddMap(Map map);

    const Maps& GetMaps() const noexcept {
        return maps_;
    }

    const Map* FindMap(const Map::Id& id) const noexcept;

    model::GameSession* AddGameSession(std::string map_id);

    model::GameSession* FindGameSessionFromMapId(const Map::Id& id) const;

    void MakeActionsAtTime(int time = 1);

    void SetRandomGenerate(bool is_random_generate){
        is_random_generate_ = is_random_generate;
    }

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
