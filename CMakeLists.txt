cmake_minimum_required(VERSION 3.10)

project(SBD_2)

# Enable testing first
enable_testing()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Debug)
add_compile_options(-g)

# Set directories
set(SRC_DIR src)
set(INCLUDE_DIR include)

# Sources
set(SOURCES
    ${SRC_DIR}/main.cpp
    ${SRC_DIR}/scoped_file.cpp
    ${SRC_DIR}/database.cpp
    ${SRC_DIR}/command_parser.cpp
)

# add_compile_definitions(DEBUG_OUTPUT=1)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_DIR})

# Add tests subdirectory
add_subdirectory(tests)
