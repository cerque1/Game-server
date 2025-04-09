#pragma once

#include <filesystem>
#include <boost/json.hpp>
#include <chrono>

#include "model.h"
#include "extra_data.h"

namespace json_loader {

struct GameInfo{
    model::Game game;
    extra_data::PossibleLootOnMapsToGenerate possible_loot;
    int retired_time;
};

GameInfo LoadGame(const std::filesystem::path& json_path);

}  // namespace json_loader
