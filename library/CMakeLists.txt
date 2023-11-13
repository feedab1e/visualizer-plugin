cmake_minimum_required(VERSION 3.25)
project(visualizer-plugin)

cmrc_add_resource_library(visualizer-plugin-resources shaders/plane.vert shaders/plane.frag NAMESPACE visualizer_plugin)
set_property(TARGET visualizer-plugin-resources PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(visualizer-plugin SHARED src/visualizer_plugin.cpp)
target_link_libraries(visualizer-plugin PRIVATE visualizer-plugin-abstraction visualizer-plugin-resources)
target_include_directories(visualizer-plugin PUBLIC include)
target_include_directories(visualizer-plugin PRIVATE private)
target_compile_features(visualizer-plugin PRIVATE cxx_std_23)
