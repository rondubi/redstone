find_package(Threads REQUIRED)

add_library(sqlite3 "sqlite3.c")
target_include_directories(sqlite3 PUBLIC ".")
target_compile_features(sqlite3 PUBLIC c_std_99)
target_link_libraries(sqlite3 PUBLIC Threads::Threads ${CMAKE_DL_LIBS})
