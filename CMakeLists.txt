cmake_minimum_required(VERSION 3.16)
project(redstone VERSION 0.1.0)

# find_package(Threads REQUIRED)
add_subdirectory("deps/fmt")
add_subdirectory("deps/spdlog2")
add_subdirectory("deps/sqlite3")

# add_library(sqlite3 "deps/sqlite3/sqlite3.c")
# target_include_directories(sqlite3 PUBLIC "deps/sqlite3")
# target_compile_features(sqlite3 PUBLIC c_std_99)
# target_link_libraries(sqlite3 PUBLIC Threads::Threads ${CMAKE_DL_LIBS})

add_executable(redstone
    "src/main.cpp"
    "src/disk/disk.cpp"
    "src/hook/hook.cpp"
    "src/hook/syscalls.cpp"
)

target_compile_features(redstone PRIVATE cxx_std_20)
target_include_directories(redstone PRIVATE "include")

target_link_libraries(redstone
    PRIVATE
        fmt::fmt
        spdlog::spdlog
        sqlite3
)

add_executable(simple
    "src/simple.cpp"
)

target_link_libraries(simple PRIVATE sqlite3)
target_compile_features(simple PRIVATE cxx_std_20)
