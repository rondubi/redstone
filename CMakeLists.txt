cmake_minimum_required(VERSION 3.16)
project(redstone VERSION 0.1.0)

add_subdirectory("deps/fmt")

set(SPDLOG_FMT_EXTERNAL ON)
add_subdirectory("deps/spdlog2")
add_subdirectory("deps/sqlite3")
add_subdirectory("deps/function_ref")
add_subdirectory("deps/tomlplusplus")

add_executable(redstone
    "src/main.cpp"
    "src/metrics.cpp"
    "src/disk/disk.cpp"
    "src/hook/hook.cpp"
    "src/hook/impls.cpp"
    "src/hook/syscalls.cpp"
    "src/net/socket.cpp"
    "src/net/datagram_socket.cpp"
    "src/net/stream_socket.cpp"
    "src/net/network.cpp"
    "src/sys/child.cpp"
    "src/sys/file.cpp"
    "src/sys/ptrace.cpp"
    # "src/redstone.cpp"
    # "src/replica.cpp"
    "src/sim/file_descriptor.cpp"
    "src/sim/machine.cpp"
    "src/sim/replica.cpp"
    "src/sim/runner/ptrace.cpp"
)

target_compile_features(redstone PRIVATE cxx_std_20)
target_include_directories(redstone PRIVATE "src")

target_link_libraries(redstone
    PRIVATE
        fmt::fmt
        spdlog::spdlog
        function-ref
        sqlite3
        tomlplusplus_tomlplusplus
)

add_executable(simple
    "src/simple.cpp"
)
target_link_libraries(simple PRIVATE sqlite3)
target_compile_features(simple PRIVATE cxx_std_20)


add_executable(socket
    "benches/socket.cpp"
)
target_compile_features(socket PRIVATE cxx_std_20)

add_executable(fib
    "benches/fib.cpp"
)
target_compile_features(fib PRIVATE cxx_std_20)

find_package(Threads REQUIRED)
add_executable(dgram
    "benches/dgram.cpp"
)
target_compile_features(dgram PRIVATE cxx_std_20)
target_link_libraries(dgram PRIVATE Threads::Threads)

add_executable(demo-echo
    "demo/echo.cpp"
)
target_compile_features(demo-echo PRIVATE cxx_std_20)

add_executable(demo-squawk
    "demo/squawk.cpp"
)
target_compile_features(demo-squawk PRIVATE cxx_std_20)