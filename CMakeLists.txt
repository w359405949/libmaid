#
# author: w<w359405949@gmail.com>
# created: 2014-08-27
option(maid_build_tests "Build all of maid's tests." OFF)
option(maid_debug "Build all of maid's tests." OFF)

PROJECT(maid)
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
include(cmake/internal_utils.cmake)
config_compiler_and_linker()

MESSAGE(STATUS "Project: ${PROJECT_NAME}")
MESSAGE(STATUS "Project Directory: ${PORJECT_SOURCE_DIR}")

file(GLOB_RECURSE header include/*.h src/include/*.h)
file(GLOB_RECURSE source src/*.cpp src/*.cc)
message("  -> Prepared: maid")

include_directories(
    ${maid_SOURCE_DIR}/include
    ${maid_SOURCE_DIR}/src/include
    ${maid_SOURCE_DIR}/gtest/include
)


set(cxx_strict "${cxx_strict} -Wno-shadow")
if (maid_debug)
    set(cxx_strict "${cxx_strict} -ggdb3 -DDEBUG=1 -O0")
endif()

cxx_shared_library(maid ${cxx_strict} ${source})

#set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/.libs)
target_link_libraries(maid uv protobuf)

################################
# INSTALL
################################
INSTALL(FILES libmaid.so DESTINATION lib)
INSTALL(DIRECTORY ${PROJECT_SOURCE_DIR}/include/maid DESTINATION include)

################################
# TEST
################################
if (maid_build_tests)
    enable_testing()

    ADD_SUBDIRECTORY(gtest)
    cxx_library(maid_main ${cxx_strict} test/maid_main)
    target_link_libraries(maid_main maid gtest)

    cxx_test(maid_controllerimpl_test maid_main)
    cxx_test(maid_channelimpl_uv_success_test maid_main test/test.pb.cc)
    cxx_test(maid_channelimpl_uv_failed_test maid_main test/test.pb.cc)
    cxx_test(maid_channel_test maid_main )
    cxx_test(maid_controller_test maid_main )
    cxx_test(maid_buffer_test maid_main )
    cxx_test(maid_remoteclosure_test maid_main test/test.pb.cc)
endif()
