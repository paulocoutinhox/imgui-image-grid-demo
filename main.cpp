#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <filesystem>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <opencv2/opencv.hpp>

namespace fs = std::filesystem;

struct ImageTexture {
    GLuint textureID;
    int width, height;
};

// Function to load texture from image
ImageTexture LoadTextureFromImage(const char* imagePath) {
    ImageTexture imgTexture = {0, 0, 0};
    int width, height, channels;
    unsigned char* data = stbi_load(imagePath, &width, &height, &channels, 0);
    if (data == nullptr) {
        std::cerr << "Error loading image: " << imagePath << std::endl;
        return imgTexture;
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

void TextAutoSizedAndCentered(const std::string& text, ImFont* font) {
    ImGuiIO& io = ImGui::GetIO();

    // Define padding
    float paddingX = 20.0f; // Horizontal padding
    float paddingY = 20.0f; // Vertical padding

    // Calculates available area considering padding
    float availableWidth = io.DisplaySize.x - 2 * paddingX;
    float availableHeight = io.DisplaySize.y - 2 * paddingY;

    // Finds the required width for the text and the number of lines
    float maxLineWidth = 0.0f;
    int lineCount = 0;
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        ImVec2 lineSize = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.0f, line.c_str());
        if (lineSize.x > maxLineWidth) {
            maxLineWidth = lineSize.x;
        }
        lineCount++;
    }

    // Adjusts the font size if the longest line is wider than the available space
    float scaleFactor = (maxLineWidth > availableWidth) ? (availableWidth / maxLineWidth) : 1.0f;
    float fontSize = font->FontSize * scaleFactor;

    // Ensures the text block fits vertically within the available height
    float totalTextHeight = fontSize * lineCount;

    if (totalTextHeight > availableHeight) {
        fontSize *= availableHeight / totalTextHeight;
    }

    // Prepares to draw the text
    ImDrawList* drawList = ImGui::GetForegroundDrawList();

    // Text and outline colors
    ImU32 textColor = IM_COL32(255, 255, 255, 255);
    ImU32 outlineColor = IM_COL32(0, 0, 0, 255);

    // Resets the stream to draw the text
    stream.clear();
    stream.seekg(0, std::ios::beg);

    // Calculates the starting y position to center the text block vertically
    float textPosY = (io.DisplaySize.y - fontSize * lineCount) / 2.0f;

    // Draws the text line by line
    while (std::getline(stream, line)) {
        ImVec2 lineSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, line.c_str());

        // Centers each line of text
        float textPosX = (io.DisplaySize.x - lineSize.x) / 2.0f;

        // Draws the outline
        float outlineThickness = 1.0f;
        for (int x = -outlineThickness; x <= outlineThickness; ++x) {
            for (int y = -outlineThickness; y <= outlineThickness; ++y) {
                if (x != 0 || y != 0) {
                    drawList->AddText(font, fontSize, ImVec2(textPosX + x, textPosY + y), outlineColor, line.c_str());
                }
            }
        }

        // Draws the line of text
        drawList->AddText(font, fontSize, ImVec2(textPosX, textPosY), textColor, line.c_str());

        // Moves to the next line
        textPosY += fontSize;
    }
}

void windowCloseCallback(GLFWwindow* window) {
    glfwDestroyWindow(window);
}

void windowKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto altF4 = (key == GLFW_KEY_W && mods == GLFW_MOD_SUPER);
    auto commandQ = (key == GLFW_KEY_F4 && mods == GLFW_MOD_ALT && action == GLFW_PRESS);

    if (altF4 || commandQ) {
        glfwDestroyWindow(window);
    }
}

int main() {
    if (!glfwInit()) {
        std::cerr << "Error initializing GLFW." << std::endl;
        return -1;
    }

    // GLFW window creation
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Main window
    GLFWwindow* window = glfwCreateWindow(1024, 768, "Image Grid with ImGui", nullptr, nullptr);
    if (window == nullptr) {
        std::cerr << "Error creating GLFW window." << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Video window
    GLFWwindow* videoWindow = nullptr;

    /*
    GLFWwindow* videoWindow = glfwCreateWindow(1024, 768, "Video Player", nullptr, nullptr);
    if (videoWindow == nullptr) {
        std::cerr << "Error creating GLFW video window." << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwSetWindowCloseCallback(videoWindow, windowCloseCallback);
    glfwSetKeyCallback(videoWindow, windowKeyCallback);
    */

    // ImGui initialization
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.IniFilename = nullptr;

    // Load fonts
    io.Fonts->AddFontDefault();
    ImFont* myFont = io.Fonts->AddFontFromFileTTF("fonts/Poppins-Bold.ttf", 500);

    if (!myFont) {
        std::cerr << "Error while load font." << std::endl;
    }

    // Load implementations for ImGui
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();

    // Load images from directory
    std::vector<ImageTexture> textures;
    std::string pathToImages = "images";

    for (const auto& entry : fs::directory_iterator(pathToImages)) {
        if (entry.is_regular_file()) {
            textures.push_back(LoadTextureFromImage(entry.path().string().c_str()));
        }
    }

    // Open video file
    cv::VideoCapture video("videos/video1.mp4");
    if (!video.isOpened()) {
        std::cerr << "Error opening video." << std::endl;
        return -1;
    }

    cv::Mat frame;
    GLuint videoTexture = 0;

    int videoWidth = 0;
    int videoHeight = 0;
    int videoStartFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoFocusOnAppearing;

    int monitorsCount;

    double fps = video.get(cv::CAP_PROP_FPS);
    auto frameDuration = std::chrono::milliseconds(static_cast<int>(1000 / fps));
    std::chrono::steady_clock::time_point lastFrameTime = std::chrono::steady_clock::now();    

    bool starting = true;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // Get video frame
        auto currentTime = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastFrameTime) >= frameDuration) {
            if (video.read(frame)) {
                // Convert BGR to RGBA
                cv::Mat frameRGBA;
                cv::cvtColor(frame, frameRGBA, cv::COLOR_BGR2RGBA);

                // Update video texture
                if (videoTexture == 0) {
                    glGenTextures(1, &videoTexture);
                    glBindTexture(GL_TEXTURE_2D, videoTexture);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                }

                glBindTexture(GL_TEXTURE_2D, videoTexture);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, frameRGBA.cols, frameRGBA.rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, frameRGBA.data);

                if (videoWidth == 0 || videoHeight == 0) {
                    videoWidth = frame.cols;
                    videoHeight = frame.rows;
                }

                lastFrameTime = currentTime;
            } else {
                // Restart video playback when reaching the end
                video.set(cv::CAP_PROP_POS_FRAMES, 0);
            }
        }

        // Main window
        glfwMakeContextCurrent(window);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Draw center text
        //TextAutoSizedAndCentered("DEUS ENVIOU\nSEU FILHO AMADO\nPRA PERDOAR\nPRA ME SALVAR", myFont);

        // Render image grid
        ImGui::Begin("Images", nullptr, ImGuiWindowFlags_NoDocking);

        if (starting) {
            ImVec2 winPos = ImVec2(ImGui::GetCursorPos().x + 50, ImGui::GetCursorPos().y + 50);

            ImGui::SetWindowPos(winPos);
            ImGui::SetWindowSize(ImVec2(400, 200));
        }

        float windowWidth = ImGui::GetContentRegionAvail().x;
        int imagesPerRow = std::max(1, static_cast<int>(windowWidth / 100.0f));
        float imageSize = windowWidth / imagesPerRow;

        for (int i = 0; i < textures.size(); ++i) {
            float aspectRatio = static_cast<float>(textures[i].width) / textures[i].height;
            ImGui::Image((void*)(intptr_t)textures[i].textureID, ImVec2(imageSize, imageSize / aspectRatio));
            if ((i + 1) % imagesPerRow != 0) ImGui::SameLine();
        }

        ImGui::End();

        // Render video        
        if (videoWindow) {
            glfwMakeContextCurrent(videoWindow);
            glfwPollEvents();
        }

        int videoFlags = videoStartFlags;
        ImVec2 videoWinPos;
        ImVec2 videoWinSize;
        GLFWmonitor** monitors = glfwGetMonitors(&monitorsCount);

        if (starting) {
            if (monitorsCount > 1) {
                int monitorPosX, monitorPosY, monitorWidth, monitorHeight;
                glfwGetMonitorWorkarea(monitors[1], &monitorPosX, &monitorPosY, &monitorWidth, &monitorHeight);

                videoWinPos = ImVec2(monitorPosX, monitorPosY);
                videoWinSize = ImVec2(monitorWidth, monitorHeight);

                videoFlags = videoFlags | ImGuiWindowFlags_NoDecoration;
            } else {
                videoWinPos = ImVec2(ImGui::GetCursorPos().x + 50, ImGui::GetCursorPos().y + 300);
                videoWinSize = ImVec2(400, 200);
            }
        } else {
            if (monitorsCount > 1) {
                int monitorPosX, monitorPosY, monitorWidth, monitorHeight;
                glfwGetMonitorWorkarea(monitors[1], &monitorPosX, &monitorPosY, &monitorWidth, &monitorHeight);

                videoWinPos = ImVec2(monitorPosX, monitorPosY);
                videoWinSize = ImVec2(monitorWidth, monitorHeight);

                videoFlags = videoFlags | ImGuiWindowFlags_NoDecoration;
            } else {
                videoWinPos = ImVec2(ImGui::GetCursorPos().x + 50, ImGui::GetCursorPos().y + 300);
                videoWinSize = ImVec2(400, 200);
            }
        }

        ImGui::Begin("Video Player", nullptr, videoFlags);
        ImGui::SetWindowPos(videoWinPos);
        ImGui::SetWindowSize(videoWinSize);

        if (videoTexture != 0) {
            // Get total window dimensions from ImGui
            ImVec2 windowSize = ImGui::GetContentRegionAvail();

            // Calculate video aspect ratio
            float videoAspectRatio = (float)videoWidth / (float)videoHeight;

            // Calculate image size based on video and window aspect ratio
            ImVec2 imageSize;
            if (windowSize.x / windowSize.y > videoAspectRatio) {
                // Window is wider than video
                imageSize.x = windowSize.y * videoAspectRatio;
                imageSize.y = windowSize.y;
            } else {
                // Window is taller than video
                imageSize.x = windowSize.x;
                imageSize.y = windowSize.x / videoAspectRatio;
            }

            // Calculate position to center the image in the window
            ImVec2 imagePos = ImVec2(
                ImGui::GetCursorPos().x + (windowSize.x - imageSize.x) * 0.5f, // X position for centering
                ImGui::GetCursorPos().y + (windowSize.y - imageSize.y) * 0.5f  // Y position for centering
                );

            // Apply calculated position
            ImGui::SetCursorPos(imagePos);

            // Render video image with adjusted size and position
            ImGui::Image((void*)(intptr_t)videoTexture, imageSize);
        }

        ImGui::End();

        // Render ImGui
        ImGui::Render();
        int displayW, displayH;
        glfwGetFramebufferSize(window, &displayW, &displayH);
        glViewport(0, 0, displayW, displayH);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        glfwSwapBuffers(window);

        starting = false;
    }

    // Cleanup
    if (videoTexture != 0) {
        glDeleteTextures(1, &videoTexture);
    }

    for (auto& texture : textures) {
        glDeleteTextures(1, &texture.textureID);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (videoWindow) {
        glfwDestroyWindow(videoWindow);
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
