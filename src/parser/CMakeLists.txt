set(CMAKE_CXX_STANDARD 20)

add_library(parser STATIC parser.cpp)
target_link_libraries(parser PUBLIC embedded_sim_lib)
