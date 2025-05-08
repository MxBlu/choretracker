cmake_minimum_required(VERSION 4.0)

project(choretracker)
aux_source_directory("src" coresrc)
add_executable(choretracker ${coresrc})

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)
target_link_libraries(choretracker PRIVATE Threads::Threads)

find_package(OpenSSL REQUIRED)
target_link_libraries(choretracker PRIVATE OpenSSL::SSL)
target_link_libraries(choretracker PRIVATE OpenSSL::Crypto)

find_package(dpp CONFIG REQUIRED)
target_link_libraries(choretracker PRIVATE dpp::dpp)

find_package(spdlog CONFIG REQUIRED)
target_link_libraries(choretracker PRIVATE spdlog::spdlog)

target_include_directories(choretracker PRIVATE "include")