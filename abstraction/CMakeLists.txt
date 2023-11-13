cmake_minimum_required(VERSION 3.25)
project(visualizer-plugin)

add_library(visualizer-plugin-abstraction INTERFACE)
target_link_libraries(visualizer-plugin-abstraction INTERFACE glfw GLEW::GLEW Boost::pfr Boost::asio)
target_include_directories(visualizer-plugin-abstraction INTERFACE include)
target_compile_features(visualizer-plugin-abstraction INTERFACE cxx_std_20)