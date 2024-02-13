cmake_minimum_required(VERSION 3.25)

project(imgui-demo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(get_cpm.cmake)

# GLFW
CPMAddPackage(
  NAME glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.3.9
  OPTIONS
    "GLFW_BUILD_DOCS OFF"
    "GLFW_BUILD_TESTS OFF"
    "GLFW_BUILD_EXAMPLES OFF"
)

# ImGui
CPMAddPackage(
  NAME imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  VERSION 1.90.2
  DOWNLOAD_ONLY YES
)

# stb_image
CPMAddPackage(
  NAME stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG master
  DOWNLOAD_ONLY YES
)

# Configure ImGui for GLFW and OpenGL3
if(imgui_ADDED AND glfw_ADDED)
  file(GLOB IMGUI_SOURCES ${imgui_SOURCE_DIR}/*.cpp)
  add_library(imgui STATIC ${IMGUI_SOURCES} ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
  target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
  target_link_libraries(imgui PUBLIC glfw)
  target_compile_definitions(imgui PUBLIC -DIMGUI_IMPL_OPENGL_LOADER_GLAD)
endif()

# Libs
find_package(OpenGL REQUIRED)

# Add stb_image.h
if(stb_ADDED)
  add_library(stb_image INTERFACE)
  target_include_directories(stb_image INTERFACE ${stb_SOURCE_DIR})
endif()

# EXE
add_executable(demo main.cpp)
target_link_libraries(demo imgui glfw ${OPENGL_gl_LIBRARY} stb_image)

file(COPY ${CMAKE_SOURCE_DIR}/images DESTINATION ${CMAKE_BINARY_DIR})

