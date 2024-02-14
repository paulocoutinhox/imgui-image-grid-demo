#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "misc/freetype/imgui_freetype.h"
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

void TextAutoSizedAndCentered(const std::string& text, ImFont* font, bool useDisplaySize) {
    ImGuiIO& io = ImGui::GetIO();

    // Define padding
    float paddingX = 20.0f; // Horizontal padding
    float paddingY = 20.0f; // Vertical padding

    ImVec2 baseSize; // Initialize base size
    ImVec2 basePos; // Initialize base position

    if (useDisplaySize) {
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            // When viewports are enabled, use the main viewport's size and position
            ImGuiViewport* mainViewport = ImGui::GetMainViewport();
            baseSize = mainViewport->Size;
            basePos = mainViewport->Pos;
        } else {
            // If viewports are not enabled, use the display size and position (0, 0)
            baseSize = io.DisplaySize;
            basePos = ImVec2(0, 0);
        }
    } else {
        // When not using display size, use the current window's size and position
        baseSize = ImGui::GetWindowSize();
        basePos = ImGui::GetWindowPos();
    }

    // Ensure there's a minimum size for drawing text
    baseSize.x = std::max(baseSize.x, 1.0f); // Minimum width
    baseSize.y = std::max(baseSize.y, 1.0f); // Minimum height

    // Calculates available area considering padding
    float availableWidth = baseSize.x - 2 * paddingX;
    float availableHeight = baseSize.y - 2 * paddingY;

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
    float textPosY = basePos.y + paddingY + (availableHeight - fontSize * lineCount) / 2.0f;

    // Draws the text line by line
    while (std::getline(stream, line)) {
        ImVec2 lineSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, line.c_str());

        // Centers each line of text
        float textPosX = basePos.x + paddingX + (availableWidth - lineSize.x) / 2.0f;

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
    io.BackendFlags |= ImGuiBackendFlags_HasMouseHoveredViewport;
    io.IniFilename = nullptr;

    // Load default font
    io.Fonts->AddFontDefault();

    // Font config
    ImFontConfig fontConfig;
    fontConfig.OversampleH = 2;
    fontConfig.OversampleV = 2;
    fontConfig.RasterizerMultiply = 1.0f;
    fontConfig.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_Monochrome;
    fontConfig.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_MonoHinting;

    ImFontConfig fontConfigBold = fontConfig;
    fontConfigBold.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags_Bold;

    // Load fonts
    ImFont* fontMain = io.Fonts->AddFontFromFileTTF("fonts/OpenSans-Regular.ttf", 18, &fontConfig, io.Fonts->GetGlyphRangesDefault());

    if (!fontMain) {
        std::cerr << "Error while load font." << std::endl;
    }

    ImFont* fontMainTitle = io.Fonts->AddFontFromFileTTF("fonts/OpenSans-Bold.ttf", 18, &fontConfigBold, io.Fonts->GetGlyphRangesDefault());

    if (!fontMainTitle) {
        std::cerr << "Error while load font." << std::endl;
    }

    ImFont* fontPlayerText = io.Fonts->AddFontFromFileTTF("fonts/Poppins-Bold.ttf", 500, &fontConfig, io.Fonts->GetGlyphRangesDefault());

    if (!fontPlayerText) {
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
        //TextAutoSizedAndCentered("DEUS ENVIOU\nSEU FILHO AMADO\nPRA PERDOAR\nPRA ME SALVAR", fontPlayerText, true);

        // Render tabs
        ImGui::PushFont(fontMain);

        ImGuiViewport* viewport = ImGui::GetMainViewport();

        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::Begin("Control Panel", nullptr, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);

        if (ImGui::BeginTabBar("TabBar")) {
            if (ImGui::BeginTabItem("Settings")) {
                // Project Folder Section
                ImGui::Dummy(ImVec2(0, 4));

                ImGui::PushFont(fontMainTitle);
                ImGui::Text("PROJECT FOLDER");
                ImGui::PopFont();

                static char folderPathBuffer[256];
                ImGui::InputText("", folderPathBuffer, IM_ARRAYSIZE(folderPathBuffer), ImGuiInputTextFlags_ReadOnly);

                ImGui::SameLine();

                if (ImGui::Button("Select Folder")) {
                    // Implement logic to open the folder selection dialog
                }

                ImGui::Text("Hint: Select the folder where your project is located");

                ImGui::Dummy(ImVec2(0, 10));
                ImGui::Separator();
                ImGui::Dummy(ImVec2(0, 10));

                // Projector Controls Section
                ImGui::PushFont(fontMainTitle);
                ImGui::Text("PROJECTOR CONTROLS");
                ImGui::PopFont();

                if (ImGui::Button("Close Projector")) {
                    // Implement logic to close the projector
                }
                ImGui::SameLine();
                if (ImGui::Button("Black Screen")) {
                    // Implement logic to set the screen to black
                }
                ImGui::SameLine();
                if (ImGui::Button("Default Screen")) {
                    // Implement logic to set the screen to default
                }

                ImGui::Dummy(ImVec2(0, 10));
                ImGui::Separator();
                ImGui::Dummy(ImVec2(0, 10));

                // Text Settings Section
                ImGui::PushFont(fontMainTitle);
                ImGui::Text("TEXT SETTINGS");
                ImGui::PopFont();

                static ImVec4 textColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
                ImGui::ColorEdit4("Text Color", (float*)&textColor, ImGuiColorEditFlags_NoInputs);

                static ImVec4 outlineColor = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
                ImGui::ColorEdit4("Outline Color", (float*)&outlineColor, ImGuiColorEditFlags_NoInputs);

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Images")) {
                const ImVec2 cellSize(120.0f, 80.0f); // Fixed size for cells
                float windowWidth = ImGui::GetContentRegionAvail().x;
                const float paddingBetweenImages = 8.0f; // Define padding between images

                // Calculate the total width used by each image, including padding
                float totalCellWidth = cellSize.x + paddingBetweenImages;

                // Adjust calculation for imagesPerRow to include padding, subtracting 1 padding since there's no padding after the last image in a row
                int imagesPerRow = static_cast<int>((windowWidth + paddingBetweenImages) / totalCellWidth);

                for (int i = 0; i < textures.size(); ++i) {
                    // If it's not the first image and reached the end of the row, start a new row
                    if (i > 0 && i % imagesPerRow == 0) {
                        ImGui::NewLine();
                    }

                    // Calculate the aspect ratio and size of the image to fit in the cell
                    float aspectRatio = static_cast<float>(textures[i].width) / textures[i].height;
                    ImVec2 imageSize = (aspectRatio > 1.0f) ? ImVec2(cellSize.x, cellSize.x / aspectRatio) : ImVec2(cellSize.y * aspectRatio, cellSize.y);

                    // Calculate padding to center the image in the cell
                    float paddingX = (cellSize.x - imageSize.x) / 2.0f;
                    float paddingY = (cellSize.y - imageSize.y) / 2.0f;

                    // Adjust cursor position for padding and center the image
                    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + paddingX);
                    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + paddingY);

                    // Draw the image
                    ImGui::Image((void*)(intptr_t)textures[i].textureID, imageSize);

                    // If it's not the end of the row, continue on the same line
                    if ((i + 1) % imagesPerRow != 0 && (i + 1) < textures.size()) {
                        ImGui::SameLine();

                        // Reset cursor position to the left edge and prepare for the next image or row
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + paddingX);
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() - paddingY);
                    }
                }

                ImGui::EndTabItem();
            }


            ImGui::EndTabBar();
        }

        ImGui::End();

        // Render image grid

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
            if (monitorsCount > 1 && io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
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
            if (monitorsCount > 1 && io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
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

        TextAutoSizedAndCentered("DEUS ENVIOU\nSEU FILHO AMADO\nPRA PERDOAR\nPRA ME SALVAR", fontPlayerText, false);

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

        // Finish
        ImGui::PopFont();

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
