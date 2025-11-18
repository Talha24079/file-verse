cmake_minimum_required(VERSION 3.10)
project(ofs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(Threads REQUIRED)

file(GLOB CORE_SOURCES "source/core/*.cpp")
file(GLOB DS_SOURCES "source/data_structures/*.cpp")

add_executable(ofs_server_run
    source/server/main.cpp
    source/server/ofs_server.cpp
    ${CORE_SOURCES}
    ${DS_SOURCES}
)

target_link_libraries(ofs_server_run PRIVATE Threads::Threads)
target_include_directories(ofs_server_run PRIVATE source/include)

add_executable(ofs_client
    source/client/client.cpp
)
target_include_directories(ofs_client PRIVATE source/include)