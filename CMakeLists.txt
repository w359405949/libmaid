#
# author: w<w359405949@gmail.com>
# created: 2014-08-27

CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

PROJECT(maid)

MESSAGE(STATUS "Project: ${PROJECT_NAME}")
MESSAGE(STATUS "Project Directory: ${PORJECT_SOURCE_DIR}")

SET(CMAKE_BUILD_TYPE Debug)
SET(CMAKE_C_FLAGS_DEBUG "-O0 -Wall -ggdb -DDEBUG")
SET(CMAKE_C_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -ggdb -DDEBUG")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

file(GLOB_RECURSE header include/*)
file(GLOB_RECURSE source src/*.cpp src/*.cc)
message("  -> Prepared: maid")

include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

add_library(${PROJECT_NAME} SHARED ${source})

set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/.libs)
target_link_libraries(${PROJECT_NAME}
    uv
    protobuf
)

INSTALL(FILES ${LIBRARY_OUTPUT_PATH}/libmaid.so DESTINATION lib)
INSTALL(FILES ${header} DESTINATION include)
