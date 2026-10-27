cmake_minimum_required(VERSION 3.19)
project(vulkannr LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)

find_package(Vulkan REQUIRED COMPONENTS glslc)

set(SHADER_SPV ${CMAKE_BINARY_DIR}/shaders/vector_add.spv)
add_custom_command(
    OUTPUT ${SHADER_SPV}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/shaders
    COMMAND Vulkan::glslc ${CMAKE_SOURCE_DIR}/shaders/vector_add.comp -o ${SHADER_SPV}
    DEPENDS ${CMAKE_SOURCE_DIR}/shaders/vector_add.comp)
add_custom_target(shaders DEPENDS ${SHADER_SPV})

add_executable(vulkannr src/main.cpp)
target_link_libraries(vulkannr PRIVATE Vulkan::Vulkan)
target_compile_definitions(vulkannr PRIVATE SHADER_DIR="${CMAKE_BINARY_DIR}/shaders")
add_dependencies(vulkannr shaders)
