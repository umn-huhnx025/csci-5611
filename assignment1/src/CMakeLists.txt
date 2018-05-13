list(APPEND SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/main.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/camera.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/particle_system.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/p_snow.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/p_fire.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/p_ball.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/p_magic.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/p_water.cpp"
)
include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
