cmake_minimum_required(VERSION 3.1)

project("FP test")

find_package(FAST REQUIRED)

include(${FAST_USE_FILE})

add_definitions(-DROOT_DIR="${PROJECT_SOURCE_DIR}")

add_executable(fastExample main.cpp GUI.hpp GUI.cpp)
add_dependencies(fastExample fast_copy)
target_link_libraries(fastExample ${FAST_LIBRARIES})
