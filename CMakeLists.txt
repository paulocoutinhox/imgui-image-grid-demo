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

# ffmpeg
set(VENDOR_FFMPEG_DIR ${CMAKE_SOURCE_DIR}/vendor/ffmpeg)

if(WIN32)
    set(VENDOR_FFMPEG_URL "")
    set(VENDOR_FFMPEG_FILE "")
    set(VENDOR_FFMPEG_ARCH_DIR "")
elseif(APPLE)
    set(VENDOR_FFMPEG_URL "https://github.com/arthenica/ffmpeg-kit/releases/download/v6.0/ffmpeg-kit-full-gpl-6.0-macos-xcframework.zip")
    set(VENDOR_FFMPEG_FILE "ffmpeg-kit-full-gpl-6.0-macos-xcframework.zip")
    set(VENDOR_FFMPEG_ARCH_DIR "macos-arm64_x86_64")
elseif(UNIX)
    set(VENDOR_FFMPEG_URL "")
    set(VENDOR_FFMPEG_FILE "")
    set(VENDOR_FFMPEG_ARCH_DIR "")
endif()

if (NOT EXISTS ${VENDOR_FFMPEG_DIR})
    file(MAKE_DIRECTORY ${VENDOR_FFMPEG_DIR})

    file(DOWNLOAD
        ${VENDOR_FFMPEG_URL}
        ${VENDOR_FFMPEG_DIR}/${VENDOR_FFMPEG_FILE}
        SHOW_PROGRESS
    )

    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xzf ${VENDOR_FFMPEG_DIR}/${VENDOR_FFMPEG_FILE}
        WORKING_DIRECTORY ${VENDOR_FFMPEG_DIR}
    )
endif()

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

file(COPY ${CMAKE_SOURCE_DIR}/images DESTINATION ${CMAKE_BINARY_DIR})

# Add ffmpeg
if(WIN32)
    # TODO
elseif(APPLE)
    find_library(FFMPEGKIT_LIB ffmpegkit PATHS ${VENDOR_FFMPEG_DIR}/ffmpegkit.xcframework/${VENDOR_FFMPEG_ARCH_DIR})
    find_library(AVCODEC_LIB libavcodec PATHS ${VENDOR_FFMPEG_DIR}/libavcodec.xcframework/${VENDOR_FFMPEG_ARCH_DIR})
    find_library(AVDEVICE_LIB libavdevice PATHS ${VENDOR_FFMPEG_DIR}/libavdevice.xcframework/${VENDOR_FFMPEG_ARCH_DIR})
    find_library(AVFILTER_LIB libavfilter PATHS ${VENDOR_FFMPEG_DIR}/libavfilter.xcframework/${VENDOR_FFMPEG_ARCH_DIR})
    find_library(AVFORMAT_LIB libavformat PATHS ${VENDOR_FFMPEG_DIR}/libavformat.xcframework/${VENDOR_FFMPEG_ARCH_DIR})
    find_library(AVUTIL_LIB libavutil PATHS ${VENDOR_FFMPEG_DIR}/libavutil.xcframework/${VENDOR_FFMPEG_ARCH_DIR})
    find_library(SWRESAMPLE_LIB libswresample PATHS ${VENDOR_FFMPEG_DIR}/libswresample.xcframework/${VENDOR_FFMPEG_ARCH_DIR})
    find_library(SWSCALE_LIB libswscale PATHS ${VENDOR_FFMPEG_DIR}/libswscale.xcframework/${VENDOR_FFMPEG_ARCH_DIR})
elseif(UNIX)
    # TODO
endif()

target_link_libraries(demo PRIVATE ${FFMPEGKIT_LIB})
target_link_libraries(demo PRIVATE ${AVCODEC_LIB})
target_link_libraries(demo PRIVATE ${AVDEVICE_LIB})
target_link_libraries(demo PRIVATE ${AVFILTER_LIB})
target_link_libraries(demo PRIVATE ${AVFORMAT_LIB})
target_link_libraries(demo PRIVATE ${AVUTIL_LIB})
target_link_libraries(demo PRIVATE ${SWRESAMPLE_LIB})
target_link_libraries(demo PRIVATE ${SWSCALE_LIB})

# Command to copy images
add_custom_command(TARGET demo POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_SOURCE_DIR}/images $<TARGET_FILE_DIR:demo>/images)

# Command to copy videos
add_custom_command(TARGET demo POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                   ${CMAKE_SOURCE_DIR}/videos $<TARGET_FILE_DIR:demo>/videos)
