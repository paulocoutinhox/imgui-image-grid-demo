extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/time.h>
#include <libavutil/imgutils.h>
}

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <filesystem>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace fs = std::filesystem;

struct ImageTexture {
    GLuint textureID;
    int width, height;
};

ImageTexture LoadTextureFromImage(const char* imagePath) {
    ImageTexture imgTexture = {0, 0, 0};
    int width, height, channels;
    unsigned char* data = stbi_load(imagePath, &width, &height, &channels, 0);
    if (data == nullptr) {
        std::cerr << "Erro ao carregar imagem: " << imagePath << std::endl;
        return imgTexture; // Retorna textureID = 0 como erro
    }

    glGenTextures(1, &imgTexture.textureID);
    glBindTexture(GL_TEXTURE_2D, imgTexture.textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, channels == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);

    imgTexture.width = width;
    imgTexture.height = height;

    return imgTexture;
}

bool InitVideo(const char* filename, AVFormatContext*& formatCtx, AVCodecContext*& codecCtx, int& videoStreamIndex) {
    if (avformat_open_input(&formatCtx, filename, nullptr, nullptr) != 0) {
        std::cerr << "Failed to open video file: " << filename << std::endl;
        return false;
    }

    if (avformat_find_stream_info(formatCtx, nullptr) < 0) {
        std::cerr << "Failed to find stream information." << std::endl;
        avformat_close_input(&formatCtx);
        return false;
    }

    videoStreamIndex = -1;
    for (unsigned i = 0; i < formatCtx->nb_streams; i++) {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStreamIndex = i;
            break;
        }
    }

    if (videoStreamIndex == -1) {
        std::cerr << "Failed to find a video stream." << std::endl;
        avformat_close_input(&formatCtx);
        return false;
    }

    AVCodecParameters* codecParams = formatCtx->streams[videoStreamIndex]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecParams->codec_id);
    codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, codecParams);
    if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
        std::cerr << "Failed to open codec." << std::endl;
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return false;
    }

    return true;
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Erro ao inicializar GLFW." << std::endl;
        return -1;
    }

#ifdef __APPLE__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(1024, 768, "Imagens em Grid com ImGui", nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "Erro ao criar janela GLFW." << std::endl;
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 410");

    std::vector<ImageTexture> textures;
    std::string pathToImages = "images";

    for (const auto& entry : fs::directory_iterator(pathToImages)) {
        if (entry.is_regular_file()) {
            textures.push_back(LoadTextureFromImage(entry.path().string().c_str()));
        }
    }

    // Initialize FFmpeg for video decoding
    AVFormatContext* formatCtx = nullptr;
    AVCodecContext* codecCtx = nullptr;
    int videoStreamIndex = -1;

    if (!InitVideo("videos/video1.mp4", formatCtx, codecCtx, videoStreamIndex)) {
        return EXIT_FAILURE;
    }

    AVFrame* frame = av_frame_alloc();
    AVPacket* packet = av_packet_alloc();
    SwsContext* swsCtx = sws_getContext(codecCtx->width, codecCtx->height, codecCtx->pix_fmt,
                                        codecCtx->width, codecCtx->height, AV_PIX_FMT_RGB24,
                                        SWS_BILINEAR, nullptr, nullptr, nullptr);
    GLuint textureID = 0;
    glGenTextures(1, &textureID);

    bool frameReady = false;
    double lastFrameTime = 0;

    // Main Loop
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        /////////////////////////////////////////////////
        // Render image grid
        /////////////////////////////////////////////////

        ImGui::Begin("Images");
        float windowWidth = ImGui::GetContentRegionAvail().x;
        int imagesPerRow = std::max(1, static_cast<int>(windowWidth / 100.0f));
        float imageSize = windowWidth / imagesPerRow;

        for (int i = 0; i < textures.size(); ++i) {
            float aspectRatio = static_cast<float>(textures[i].width) / textures[i].height;
            ImGui::Image((void*)(intptr_t)textures[i].textureID, ImVec2(imageSize, imageSize / aspectRatio));
            if ((i + 1) % imagesPerRow != 0) ImGui::SameLine();
        }
        ImGui::End();

        /////////////////////////////////////////////////
        // Render video
        /////////////////////////////////////////////////

        if (av_read_frame(formatCtx, packet) >= 0) {
            if (packet->stream_index == videoStreamIndex) {
                if (avcodec_send_packet(codecCtx, packet) == 0) {
                    while (avcodec_receive_frame(codecCtx, frame) == 0) {
                        // Ensure the texture ID is generated outside the loop, only once
                        if (textureID == 0) {
                            glGenTextures(1, &textureID);
                            glBindTexture(GL_TEXTURE_2D, textureID);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                        }

                        // Prepare buffer for RGB data
                        uint8_t* dstData[1]; // data pointers
                        int dstLinesize[1]; // linesize
                        int outputWidth = codecCtx->width;
                        int outputHeight = codecCtx->height;
                        av_image_alloc(dstData, dstLinesize, outputWidth, outputHeight, AV_PIX_FMT_RGB24, 1);

                        // Convert the frame from its native format to RGB
                        sws_scale(swsCtx, frame->data, frame->linesize, 0, codecCtx->height, dstData, dstLinesize);

                        // Update the OpenGL texture with the new frame
                        glBindTexture(GL_TEXTURE_2D, textureID);
                        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, outputWidth, outputHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, dstData[0]);

                        // Free the image buffer after use
                        av_freep(&dstData[0]);

                        // Draw the video in a Dear ImGui window
                        ImGui::Begin("Video Player");
                        ImGui::Image((void*)(intptr_t)textureID, ImVec2(outputWidth, outputHeight));
                        ImGui::End();
                    }
                }
                av_packet_unref(packet);
            }
        } else {
            // If the end of the video file is reached, seek back to the beginning
            av_seek_frame(formatCtx, videoStreamIndex, 0, AVSEEK_FLAG_BACKWARD);
            avcodec_flush_buffers(codecCtx);
        }

        // Draw the video in a Dear ImGui window
        if (frameReady) {
            ImGui::Begin("Video Player");
            ImGui::Image((void*)(intptr_t)textureID, ImVec2(codecCtx->width, codecCtx->height));
            ImGui::End();
        }

        // render frame
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    /////////////////////////////////////////////////////////////
    // dealloc video
    /////////////////////////////////////////////////////////////

    // Free the frame and packet
    av_frame_free(&frame);
    av_packet_free(&packet);

    // Free the SwsContext
    sws_freeContext(swsCtx);

    // Close the codec context
    if (codecCtx) {
        avcodec_close(codecCtx);
        avcodec_free_context(&codecCtx);
    }

    // Close the video file/format context
    if (formatCtx) {
        avformat_close_input(&formatCtx);
    }

    // Delete the OpenGL texture
    if (textureID != 0) {
        glDeleteTextures(1, &textureID);
    }

    /////////////////////////////////////////////////////////////
    // dealloc textures
    /////////////////////////////////////////////////////////////
    for (auto& tex : textures) {
        glDeleteTextures(1, &tex.textureID);
    }

    /////////////////////////////////////////////////////////////
    // dealloc imgui
    /////////////////////////////////////////////////////////////

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
