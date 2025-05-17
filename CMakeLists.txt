cmake_minimum_required(VERSION 3.10)
project(htx-grabber LANGUAGES CXX)

########################## App ##################################
# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

# Find dependencies
find_package(Boost REQUIRED COMPONENTS unit_test_framework system)
find_package(jsoncpp REQUIRED)

# Recursively find all source files
file(GLOB_RECURSE SOURCES "src/*.cpp")

# (Optional) Main executable (exclude if you only want tests)
add_executable(main_app ${SOURCES})
target_include_directories(main_app PRIVATE include)
target_link_libraries(main_app PRIVATE jsoncpp_lib ssl crypto z Boost::system Threads::Threads)

# Create object library of main app sources
add_library(app_lib STATIC
    ${SOURCES}
)
target_include_directories(app_lib PUBLIC include /usr/include/jsoncpp)

######################### tests ##########################
file(GLOB_RECURSE TEST_SOURCES "tests/*.cpp")

include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
FetchContent_MakeAvailable(googletest)

include(CTest)
find_package(GTest REQUIRED)
include(GoogleTest)

FetchContent_Declare(
  fmt
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 8.1.1  # Use the latest stable release
)
FetchContent_MakeAvailable(fmt)

# Test executable
add_executable(tests ${TEST_SOURCES})
target_include_directories(tests PRIVATE include /usr/include/jsoncpp ${fmt_SOURCE_DIR}/include)
target_link_libraries(tests
    PRIVATE
    fmt::fmt
    app_lib
    GTest::GTest
    GTest::Main
    jsoncpp_lib

    ssl crypto z Boost::system Threads::Threads
)
target_include_directories(tests PRIVATE include)

# Enable testing support
set(CMAKE_BUILD_TYPE Debug)
enable_testing()
gtest_discover_tests(tests)

# Add this to track header changes
set_property(TARGET main_app PROPERTY CXX_STANDARD 17)
set_property(TARGET tests PROPERTY CXX_STANDARD 17)
