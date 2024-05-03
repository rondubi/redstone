cmake_minimum_required(VERSION 3.20)
project(redstone VERSION 0.1.0)

find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

add_executable(redstone "src/main.cpp")

target_compile_features(redstone PRIVATE cxx_std_20)
target_include_directories(redstone PRIVATE "include")
target_link_libraries(redstone PRIVATE fmt::fmt)
target_link_libraries(redstone PRIVATE spdlog::spdlog)
