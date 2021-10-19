cmake_minimum_required(VERSION 3.0.0)
project(YepClock VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 14)
set(GLAD_DIR ../../libs/glad-gl3.0)
set(GLFW_DIR ../../libs/glfw-3.3.4)

include_directories(${GLAD_DIR}/include)

add_subdirectory(${GLFW_DIR} glfw)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

FILE(GLOB SrcFiles src/*)

add_executable(${PROJECT_NAME} WIN32
    ${SrcFiles}
    ${GLAD_DIR}/src/glad.c)

target_link_libraries(YepClock glfw)
