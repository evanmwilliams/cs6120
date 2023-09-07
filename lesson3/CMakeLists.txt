#######
cmake_minimum_required(VERSION 3.12)
set (CMAKE_CXX_STANDARD 17)

project(ProfileProject)
find_package(nlohmann_json 3.11.2 REQUIRED)

add_executable(dce dce.cpp)
add_executable(lvn lvn.cpp)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

target_link_libraries(dce PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(lvn PRIVATE nlohmann_json::nlohmann_json)
