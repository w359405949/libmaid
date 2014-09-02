#
# author: w<w359405949@gmail.com>
# created: 2014-08-27

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(maid)

MESSAGE(STATUS "Project: ${PROJECT_NAME}")
MESSAGE(STATUS "Project Directory: ${PORJECT_SOURCE_DIR}")

SET(CMAKE_BUILD_TYPE Debug)
SET(CMAKE_C_FLAGS_DEBUG "-g -Wall")
# SET(CMAKE_CXX_FLAGS "std=c++0x")
SET(CMAKE_CXX_FLAGS_DEBUG "-g -ggdb -Wall")

file(GLOB_RECURSE source src/*.cpp src/*.h src/*.cc)
message("  -> Prepared: maid")

include_directories(
    ${PROJECT_SOURCE_DIR}/include/
    ${PROJECT_SOURCE_DIR}/deps/libev/
    ${PROJECT_SOURCE_DIR}/deps/protobuf/src/
)

link_directories(
    ${PROJECT_SOURCE_DIR}/deps/libev/.libs/
    ${PROJECT_SOURCE_DIR}/deps/protobuf/src/.libs/
)


add_library(${PROJECT_NAME} SHARED ${source})

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/.libs/)
target_link_libraries(${PROJECT_NAME}
    ev
    protobuf
)

#me_component add_subdirectory(src)
