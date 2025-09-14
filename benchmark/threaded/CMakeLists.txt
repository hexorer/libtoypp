add_executable(${PROJECT_NAME}-benchmark-threaded-doublebuffer doublebuffer.cpp)
target_link_libraries(${PROJECT_NAME}-benchmark-threaded-doublebuffer PRIVATE ${PROJECT_NAME}-benchmark-options)
