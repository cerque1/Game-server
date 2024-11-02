cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)
set(CONAN_DISABLE_CHECK_COMPILER TRUE CACHE BOOL "" FORCE )

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

find_package(Boost 1.86.0 REQUIRED)
if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_compile_definitions(BOOST_BEAST_USE_STD_STRING_VIEW)
add_executable(game_server
	src/main.cpp
	src/http_server.cpp
	src/http_server.h
	src/sdk.h
	src/model.h
	src/model.cpp
	src/tagged.h
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
	src/player.cpp)

target_link_libraries(game_server PRIVATE CONAN_PKG::boost Threads::Threads)
