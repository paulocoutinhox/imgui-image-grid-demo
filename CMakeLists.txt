cmake_minimum_required(VERSION 3.25)

project(imgui-demo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(get_cpm.cmake)

# poco
CPMAddPackage(
    NAME Poco
    GITHUB_REPOSITORY pocoproject/poco
    GIT_TAG poco-1.13.2
    OPTIONS
        "BUILD_SHARED_LIBS NO"
        "POCO_UNBUNDLED OFF"
        "ENABLE_TESTS OFF"
        "ENABLE_NET ON"
        "ENABLE_NETSSL OFF"
        "ENABLE_XML OFF"
        "ENABLE_MONGODB OFF"
        "ENABLE_DATA_SQLITE OFF"
        "ENABLE_REDIS OFF"
        "ENABLE_PROMETHEUS OFF"
        "ENABLE_PDF OFF"
        "ENABLE_SEVENZIP OFF"
        "ENABLE_ZIP OFF"
        "ENABLE_CPPPARSER OFF"
        "ENABLE_POCODOC OFF"
        "ENABLE_PAGECOMPILER OFF"
        "ENABLE_PAGECOMPILER_FILE2PAGE OFF"
        "ENABLE_ACTIVERECORD OFF"
        "ENABLE_ACTIVERECORD_COMPILER OFF"
        "ENABLE_DATA OFF"
        "ENABLE_JWT OFF"
        "ENABLE_UTIL ON"
        "ENABLE_APACHECONNECTOR OFF"
)

# GLFW
CPMAddPackage(
  NAME glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG master
  OPTIONS
    "GLFW_BUILD_DOCS OFF"
    "GLFW_BUILD_TESTS OFF"
    "GLFW_BUILD_EXAMPLES OFF"
)

# ImGui
CPMAddPackage(
  NAME imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG docking
  DOWNLOAD_ONLY YES
)

# stb_image
CPMAddPackage(
  NAME stb
  GIT_REPOSITORY https://github.com/nothings/stb.git
  GIT_TAG master
  DOWNLOAD_ONLY YES
)

# json
CPMAddPackage("gh:nlohmann/json@3.11.3")

# libzip
# CPMAddPackage(
#   NAME libzip
#   GIT_REPOSITORY https://github.com/nih-at/libzip.git
#   GIT_TAG "v1.10.1"
#   OPTIONS
#     "ENABLE_COMMONCRYPTO OFF"
#     "ENABLE_GNUTLS OFF"
#     "ENABLE_MBEDTLS ON"
#     "ENABLE_OPENSSL OFF"
#     "ENABLE_WINDOWS_CRYPTO OFF"
#     "BUILD_TOOLS OFF"
#     "BUILD_REGRESS OFF"
#     "BUILD_OSSFUZZ OFF"
#     "BUILD_EXAMPLES OFF"
#     "BUILD_DOC OFF"
#     "BUILD_SHARED_LIBS OFF"
#     "LIBZIP_DO_INSTALL OFF"
# )

# opencv
find_package(OpenCV REQUIRED)

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
target_link_libraries(demo PUBLIC imgui glfw ${OPENGL_gl_LIBRARY} stb_image)

# OpenCV
# Inclua os diretórios de cabeçalho do OpenCV
include_directories(${OpenCV_INCLUDE_DIRS})

# Link com o OpenCV
target_link_libraries(demo PUBLIC ${OpenCV_LIBS})

# Link com zip
#target_link_libraries(demo PUBLIC libzip::zip)

# Link com poco
target_link_libraries(demo PUBLIC Poco::Foundation Poco::Net Poco::Util)

# Link com json
target_link_libraries(demo PUBLIC nlohmann_json::nlohmann_json)

# vendor
include_directories(${CMAKE_SOURCE_DIR}/vendor)

# Command to copy images
add_custom_command(TARGET demo POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_SOURCE_DIR}/images $<TARGET_FILE_DIR:demo>/images)

# Command to copy videos
add_custom_command(TARGET demo POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_SOURCE_DIR}/videos $<TARGET_FILE_DIR:demo>/videos)

# Command to copy fonts
add_custom_command(TARGET demo POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_SOURCE_DIR}/fonts $<TARGET_FILE_DIR:demo>/fonts)

# Command to copy web
add_custom_command(TARGET demo POST_BUILD
                  COMMAND ${CMAKE_COMMAND} -E copy_directory
                  ${CMAKE_SOURCE_DIR}/web $<TARGET_FILE_DIR:demo>/web)
