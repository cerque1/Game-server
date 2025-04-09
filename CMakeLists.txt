cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)
set(CONAN_DISABLE_CHECK_COMPILER TRUE CACHE BOOL "" FORCE )
# set(CMAKE_CXX_FLAGS "-g")
# set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

find_package(Boost 1.86.0 REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# add_definitions("/bigobj")
add_compile_definitions(BOOST_BEAST_USE_STD_STRING_VIEW)

add_executable(game_server
	src/main.cpp
	src/http_server.cpp
	src/http_server.h
	src/sdk.h
	src/model.h
	src/model.cpp
	src/tagged.h
	src/tagged_uuid.h
	src/tagged_uuid.cpp
	src/boost_json.cpp
	src/json_loader.h
	src/json_loader.cpp
	src/request_handler.cpp
	src/request_handler.h
	src/log_utils.h
	src/log_utils.cpp
	src/api_request_handler.h
	src/api_request_handler.cpp
	src/request_handle_utils.h
	src/request_handle_utils.cpp
	src/player.h
	src/player.cpp
	src/json_utils.h 
	src/json_utils.cpp
	src/loot_generator.h
	src/loot_generator.cpp
	src/extra_data.h
	src/extra_data.cpp
	src/collision_detector.h
	src/collision_detector.cpp
	src/geom.h
	src/objects_collector.h
	src/objects_collector.cpp
	src/model_serialization.h
	src/serializing_listener.h
	src/serializing_listener.cpp
	src/records.h
	src/postgres.h
	src/postgres.cpp
	src/connection_pool.h)

if(WIN32)
	target_compile_definitions(game_server PRIVATE _WIN32_WINNT=0x0A00)
endif()

target_link_libraries(game_server PRIVATE CONAN_PKG::boost Threads::Threads CONAN_PKG::libpq CONAN_PKG::libpqxx)