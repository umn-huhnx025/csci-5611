cmake_minimum_required(VERSION 3.1)

project(assignment2 C CXX)
include(src/CMakeLists.txt)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -g -O3")

add_executable(assignment2 glad/glad.c ${SOURCES})

find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)

include_directories(${PROJECT_SOURCE_DIR} ${OPENGL_INCLUDE_DIRS}  ${SDL2_INCLUDE_DIR})

target_link_libraries(assignment2 ${OPENGL_LIBRARIES} ${SDL2_LIBRARY} ${CMAKE_DL_LIBS})
