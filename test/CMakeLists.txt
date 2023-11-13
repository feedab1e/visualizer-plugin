cmake_minimum_required(VERSION 3.25)
project(visualizer-plugin)

cmrc_add_resource_library(default_renderers-resources shaders/cube.vert shaders/cube.frag NAMESPACE default_renderers)
set_property(TARGET default_renderers-resources PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(default_renderers SHARED src/default-renderers/cube.cpp)
target_link_libraries(default_renderers PRIVATE default_renderers-resources visualizer-plugin visualizer-plugin-abstraction)
add_executable(testfile src/testfile.cpp)
add_executable(testfile_autoload src/testfile_autoload.cpp)
target_link_libraries(testfile_autoload default_renderers visualizer-plugin)