cmake_minimum_required(VERSION 3.8)

project(toypp CXX)

set(CMAKE_CXX_STANDARD 17)

option(TOYPP_TESTS "Build and perform tests" OFF)
option(TOYPP_COVERAGE "Add coverage" OFF)

set(LIBTOYPP_PUBLIC_HEADER_DIR "${PROJECT_SOURCE_DIR}/include/")

add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(
  ${PROJECT_NAME} INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if (TOYPP_TESTS)
  enable_testing()
  add_subdirectory("tests")
endif()
