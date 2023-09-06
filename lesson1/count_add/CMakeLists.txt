#######
cmake_minimum_required(VERSION 3.12)
project(ProfileProject)
find_package(nlohmann_json 3.11.2 REQUIRED)

add_executable(count_add count_add.cpp)
target_link_libraries(count_add PRIVATE nlohmann_json::nlohmann_json)
