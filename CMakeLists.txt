cmake_minimum_required(VERSION 3.16)
project(milesChess LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(MILESCHESS_NATIVE "Tune for the host CPU (-march=native)" ON)

add_library(milesChess_flags INTERFACE)
target_compile_features(milesChess_flags INTERFACE cxx_std_20)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(milesChess_flags INTERFACE -Wall -Wno-unused-variable)
    if(MILESCHESS_NATIVE)
        target_compile_options(milesChess_flags INTERFACE -march=native)
    else()
        target_compile_options(milesChess_flags INTERFACE -mpopcnt -mbmi -mbmi2)
    endif()
endif()

find_package(Threads REQUIRED)

add_library(milesChess_core STATIC
    src/MoveGen.cpp
    src/Position.cpp
    src/TableBase.cpp
    src/UCI.cpp
)
target_include_directories(milesChess_core PUBLIC src)
target_link_libraries(milesChess_core PUBLIC Threads::Threads milesChess_flags)

add_executable(engine src/main.cpp)
target_link_libraries(engine PRIVATE milesChess_core)

include(CTest)
if(BUILD_TESTING AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
endif()
