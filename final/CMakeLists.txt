cmake_minimum_required(VERSION 3.1)

project(final C CXX)
include(src/CMakeLists.txt)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo)
endif()

add_executable(${PROJECT_NAME} glad/glad.c ${SOURCES})

set(SHADER_DIR "${PROJECT_SOURCE_DIR}/src/shaders")
set(MODEL_DIR "${PROJECT_SOURCE_DIR}/models")
set(TEXTURE_DIR "${PROJECT_SOURCE_DIR}/textures")
configure_file(
  "${PROJECT_SOURCE_DIR}/src/include/config.h.in"
  "${PROJECT_SOURCE_DIR}/src/include/config.h"
)

find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)

if (WIN32)
  set_target_properties( ${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
  set_target_properties( ${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
  set_target_properties( ${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)
  set_target_properties( ${PROJECT_NAME} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/bin)
  get_filename_component(SDL2_LIB_DIR ${SDL2MAIN_LIBRARY} DIRECTORY)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
	  "${SDL2_LIB_DIR}/SDL2.dll" ${CMAKE_BINARY_DIR}/bin/SDL2.dll)
endif()

# find_package(OpenMP REQUIRED)
# if (OpenMP_FOUND)
#   set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
#   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
# endif()

include_directories(${PROJECT_SOURCE_DIR} ${OPENGL_INCLUDE_DIRS}  ${SDL2_INCLUDE_DIR})

target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES} ${SDL2_LIBRARY} ${CMAKE_DL_LIBS})
