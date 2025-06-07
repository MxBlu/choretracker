cmake_minimum_required(VERSION 4.0)

project(choretracker)
aux_source_directory("src" coresrc)
add_executable(choretracker ${coresrc})

target_compile_features(choretracker PRIVATE cxx_std_23)
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

find_package(bsoncxx CONFIG REQUIRED)
find_package(mongocxx CONFIG REQUIRED)
target_link_libraries(choretracker PRIVATE 
    $<IF:$<TARGET_EXISTS:mongo::bsoncxx_static>,mongo::bsoncxx_static,mongo::bsoncxx_shared>
    $<IF:$<TARGET_EXISTS:mongo::mongocxx_static>,mongo::mongocxx_static,mongo::mongocxx_shared>)

find_package(httplib CONFIG REQUIRED)
target_link_libraries(choretracker PRIVATE httplib::httplib)

find_package(Crow CONFIG REQUIRED)
target_link_libraries(choretracker PRIVATE Crow::Crow asio::asio)

find_package(stduuid CONFIG REQUIRED)
target_link_libraries(choretracker PRIVATE stduuid)

target_include_directories(choretracker PRIVATE "include")

if(WIN32)
    target_compile_definitions(choretracker PRIVATE
        WIN32_LEAN_AND_MEAN
        _WIN32_WINNT=0x0A00
    )
endif()

IF (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Debug mode: Enabling AddressSanitizer and other debug tools")
    target_compile_options(choretracker PRIVATE -fsanitize=address -fno-omit-frame-pointer -g)
    target_link_options(choretracker PRIVATE -fsanitize=address)
ENDIF()