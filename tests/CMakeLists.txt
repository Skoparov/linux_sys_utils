cmake_minimum_required(VERSION 2.8)

set(TEST_PROJECT "linux_utils_unit_tests")
PROJECT(${TEST_PROJECT})

add_definitions("-std=c++11")
set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
set(CMAKE_EXE_LINKER_FLAGS "-Wno-deprecated-declarations")

find_library(PTHREAD pthread)
find_package(Boost COMPONENTS unit_test_framework filesystem regex REQUIRED)

set( SOURCE_DIR ../ )
file( GLOB SOURCES "tests.cpp"
                   "${SOURCE_DIR}/sys*.h"
                   "${SOURCE_DIR}/impl/sys*.h"
                   "${SOURCE_DIR}/aux_methods.h"
                   "${SOURCE_DIR}/impl/sys*.cpp"
                   "${SOURCE_DIR}/impl/execute_sys_command.*"
                   "${SOURCE_DIR}/impl/aux_methods.cpp"  )

include_directories(${SOURCE_DIR} ${Boost_INCLUDE_DIRS})
add_definitions(-DBOOST_TEST_DYN_LINK)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/Bin/${CMAKE_BUILD_TYPE})

add_executable(${TEST_PROJECT} ${SOURCES})
link_directories (${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
target_link_libraries(${TEST_PROJECT} ${PTHREAD} ${Boost_LIBRARIES} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY} )

# Copy run.sh & Resources
add_custom_command(TARGET ${TEST_PROJECT} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_CURRENT_SOURCE_DIR}/run.sh" ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} COMMENT "Copying run.sh")

set(CONFIG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Resources)
get_filename_component(CONFIG_DIR_NAME ${CONFIG_DIR} NAME)
set(CONFIG_DIR_OUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CONFIG_DIR_NAME})
add_custom_command(TARGET ${TEST_PROJECT} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CONFIG_DIR_NAME}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CONFIG_DIR} ${CONFIG_DIR_OUT} COMMENT "Copying config dir")




