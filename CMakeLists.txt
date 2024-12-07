cmake_minimum_required(VERSION 3.10)

project(SBD_2)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SRC_DIR src)
set(INCLUDE_DIR include)

set(SOURCES
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/scoped_file.cpp
    ${SRC_DIR}/database.cpp
    ${SRC_DIR}/command_parser.cpp
)

set(CMAKE_BUILD_TYPE Debug)
add_compile_options(-g)

add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIR})