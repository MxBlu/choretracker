cmake_minimum_required(VERSION 4.0)
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

project(choretracker)
aux_source_directory("src" coresrc)
add_executable(choretracker ${coresrc})

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)


find_package(DPP REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(spdlog REQUIRED)

target_include_directories(choretracker PRIVATE
    "include"
    ${DPP_INCLUDE_DIR}
    ${OPENSSL_INCLUDE_DIR}
    ${SPDLOG_INCLUDE_DIR})
target_link_libraries(choretracker
    ${DPP_LIBRARIES}
    ${OPENSSL_CRYPTO_LIBRARY}
    ${OPENSSL_SSL_LIBRARY}
    ${SPDLOG_LIBRARY})