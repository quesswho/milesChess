cmake_minimum_required(VERSION 3.16)
project(milesChess LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(MILESCHESS_NATIVE "Tune for the host CPU (-march=native)" ON)

add_executable(engine
    src/main.cpp
    src/MoveGen.cpp
    src/Position.cpp
    src/TableBase.cpp
    src/UCI.cpp
)

target_include_directories(engine PRIVATE src)

find_package(Threads REQUIRED)
target_link_libraries(engine PRIVATE Threads::Threads)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(engine PRIVATE -Wall -Wno-unused-variable)
    if(MILESCHESS_NATIVE)
        target_compile_options(engine PRIVATE -march=native)
    else()
        # Baseline the engine actually needs: popcnt (COUNT_BIT) and bmi (tzcnt).
        target_compile_options(engine PRIVATE -mpopcnt -mbmi -mbmi2)
    endif()
endif()
