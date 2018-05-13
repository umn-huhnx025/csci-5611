list(APPEND SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/main.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/camera.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/crowd.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/graph.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/vertex.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/edge.cpp"
)
include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
