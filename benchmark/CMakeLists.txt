find_package(benchmark CONFIG REQUIRED)

add_library(${PROJECT_NAME}-benchmark-options INTERFACE)

target_link_libraries(${PROJECT_NAME}-benchmark-options
    INTERFACE
        ${PROJECT_NAME}::${PROJECT_NAME}
        benchmark::benchmark
        benchmark::benchmark_main)

add_subdirectory(threaded)
