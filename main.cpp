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
    if (data == NULL) {
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

    GLFWwindow* window = glfwCreateWindow(1024, 768, "Imagens em Grid com ImGui", NULL, NULL);
    if (window == NULL) {
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
            textures.push_back(LoadTextureFromImage(entry.path().c_str()));
        }
    }

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Imagens");
        float windowWidth = ImGui::GetContentRegionAvail().x;
        int imagesPerRow = std::max(1, static_cast<int>(windowWidth / 100.0f));
        float imageSize = windowWidth / imagesPerRow;

        for (int i = 0; i < textures.size(); ++i) {
            float aspectRatio = static_cast<float>(textures[i].width) / textures[i].height;
            ImGui::Image((void*)(intptr_t)textures[i].textureID, ImVec2(imageSize, imageSize / aspectRatio));
            if ((i + 1) % imagesPerRow != 0) ImGui::SameLine();
        }
        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    for (auto& tex : textures) {
        glDeleteTextures(1, &tex.textureID);
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
