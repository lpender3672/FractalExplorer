cmake_minimum_required(VERSION 3.10)
project(fractals VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Default to release builds
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/../SFML-2.5.1")

set(FRAG_SHADER_FILE "${CMAKE_SOURCE_DIR}/src/frag.glsl")
set(VERT_SHADER_FILE "${CMAKE_SOURCE_DIR}/src/vert.glsl")

find_package(SFML 2 REQUIRED COMPONENTS graphics system window)
find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)

set(SFML_LIBS sfml-graphics sfml-system sfml-window)

file(GLOB SOURCES src/*.cpp)
add_executable(${PROJECT_NAME} WIN32 ${SOURCES})
target_include_directories(${PROJECT_NAME} PRIVATE "include" "lib" ${OPENGL_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} ${SFML_LIBS} ${OPENGL_INCLUDE_DIRS})
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)
if (UNIX)
   target_link_libraries(${PROJECT_NAME} pthread)
endif (UNIX)


if(MSVC)
   foreach(lib ${SFML_LIBS})
      get_target_property(lib_path ${lib} LOCATION)
      file(COPY ${lib_path} DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
   endforeach()
endif()
