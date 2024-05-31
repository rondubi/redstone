cmake_minimum_required(VERSION 3.16)
project(redstone VERSION 0.1.0)

add_subdirectory("deps/fmt")

set(SPDLOG_FMT_EXTERNAL ON)
add_subdirectory("deps/spdlog2")
add_subdirectory("deps/sqlite3")

add_executable(redstone
    "src/main.cpp"
    "src/disk/disk.cpp"
    "src/hook/hook.cpp"
    "src/hook/syscalls.cpp"
    "src/sys/child.cpp"
    "src/sys/ptrace.cpp"
    "src/redstone.cpp"
    "src/replica.cpp"
    "src/sim/runner/ptrace.cpp"
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
