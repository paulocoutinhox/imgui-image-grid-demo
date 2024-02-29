#include <iostream>
#include <vector>
#include <chrono>
#include <filesystem>
#include <algorithm>

#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "misc/freetype/imgui_freetype.h"
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <opencv2/opencv.hpp>
#include <opencv2/objdetect.hpp>

namespace fs = std::filesystem;

struct ImageTexture
{
    GLuint textureID;
    int width, height;
};

// dialog
#include "portable-file-dialogs/portable-file-dialogs.h"

// json
#include <nlohmann/json.hpp>
using json = nlohmann::json;

// HTTP
#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Util/ServerApplication.h>
#include <Poco/Net/NetworkInterface.h>
#include <Poco/URI.h>
#include <Poco/Path.h>
#include <Poco/File.h>
#include <fstream>

using namespace Poco::Net;
using namespace Poco::Util;
using namespace std;

class FileRequestHandler : public HTTPRequestHandler
{
public:
    void handleRequest(HTTPServerRequest &req, HTTPServerResponse &resp) override
    {
        // Exemplo: http://localhost:8080/rcontrol/?api_url=http://localhost:8080/api

        // O diretório base onde os arquivos estão localizados
        const std::string basePath = "./web"; // Assegura que aponta para a pasta 'web' no diretório atual

        // Extrai o caminho do URI da solicitação e forma o caminho do arquivo
        std::string requestedPath = req.getURI();

        // Remove os parâmetros de consulta (tudo após '?')
        size_t queryStart = requestedPath.find('?');
        if (queryStart != std::string::npos)
        {
            requestedPath = requestedPath.substr(0, queryStart);
        }

        // Prevenção simples contra Directory Traversal Attack
        if (requestedPath.find("..") != std::string::npos)
        {
            resp.setStatus(HTTPResponse::HTTP_FORBIDDEN);
            resp.send() << "403 - Forbidden";
            return;
        }

        if (requestedPath.back() == '/')
        {
            requestedPath += "index.html";
        }
        else if (requestedPath == "/")
        { // Se nenhum caminho for especificado, usar '/index.html'
            requestedPath = "/index.html";
        }

        // Constrói o caminho final do arquivo solicitado
        std::string fullPath = basePath + requestedPath;

        Poco::File file(fullPath);

        if (file.exists())
        {
            if (file.isDirectory())
            {
                fullPath = Poco::Path(fullPath).append("index.html").toString();
                file = Poco::File(fullPath);
            }
        }

        if (file.exists() && file.isFile())
        {
            // Determina o tipo de conteúdo baseado na extensão do arquivo
            std::string contentType = "text/plain"; // Default content type
            if (fullPath.find(".html") != std::string::npos)
            {
                contentType = "text/html";
            }
            else if (fullPath.find(".js") != std::string::npos)
            {
                contentType = "application/javascript";
            }
            else if (fullPath.find(".css") != std::string::npos)
            {
                contentType = "text/css";
            }
            else if (fullPath.find(".png") != std::string::npos)
            {
                contentType = "image/png";
            }
            else if (fullPath.find(".jpg") != std::string::npos || fullPath.find(".jpeg") != std::string::npos)
            {
                contentType = "image/jpeg";
            }

            // Lê e envia o arquivo
            std::ifstream fileStream(fullPath, std::ifstream::binary);
            std::ostringstream ss;
            ss << fileStream.rdbuf(); // Lê todo o conteúdo do arquivo
            std::string content = ss.str();

            resp.setStatus(HTTPResponse::HTTP_OK);
            resp.setContentType(contentType);
            std::ostream &out = resp.send();
            out << content;
        }
        else
        {
            // Arquivo não encontrado
            resp.setStatus(HTTPResponse::HTTP_NOT_FOUND);
            resp.send() << "404 - Not Found";
        }
    }
};

class APIRequestHandler : public HTTPRequestHandler
{
public:
    void handleRequest(HTTPServerRequest &req, HTTPServerResponse &resp) override
    {
        resp.setStatus(HTTPResponse::HTTP_OK);
        resp.setContentType("application/json");

        ostream &out = resp.send();
        out << R"({"message": "This is a JSON response from API"})";
        out.flush();
    }
};

class RequestHandlerFactory : public HTTPRequestHandlerFactory
{
public:
    HTTPRequestHandler *createRequestHandler(const HTTPServerRequest &request) override
    {
        Poco::URI uri(request.getURI());
        if (uri.getPath().find("/api") == 0)
        {
            return new APIRequestHandler;
        }
        else
        {
            return new FileRequestHandler;
        }
    }
};

class WebServer
{
public:
    bool serverRunning = false;
    HTTPServer *server = nullptr;

    WebServer() : server(nullptr) {}

    void start(int port)
    {
        if (serverRunning)
            return; // Impede iniciar múltiplas vezes
        try
        {
            server = new HTTPServer(new RequestHandlerFactory, ServerSocket(port), new HTTPServerParams);
            server->start();
            std::cout << "Server started on port " << port << "." << std::endl;
            serverRunning = true;
        }
        catch (Poco::Exception &e)
        {
            std::cerr << "Error: " << e.displayText() << std::endl;
        }
    }

    void stop()
    {
        if (!serverRunning)
            return; // Impede parar se não estiver rodando
        try
        {
            if (server)
            {
                server->stop();
                delete server;
                server = nullptr;
                std::cout << "Server stopped." << std::endl;
                serverRunning = false;
            }
        }
        catch (Poco::Exception &e)
        {
            std::cerr << "Error: " << e.displayText() << std::endl;
        }
    }

    std::string getLocalIPAddress()
    {
        // Obtém todas as interfaces de rede
        Poco::Net::NetworkInterface::Map map = Poco::Net::NetworkInterface::map();
        std::string localIP = "127.0.0.1"; // Endereço padrão caso não encontre uma interface externa

        for (const auto &m : map)
        {
            // Procura por uma interface de rede IPv4 que não seja loopback (127.0.0.1), esteja ativa e não seja um gateway
            if (!m.second.isLoopback() && m.second.supportsIPv4() && m.second.isUp())
            {
                const auto &ips = m.second.addressList();
                for (const auto &ipa : ips)
                {
                    if (ipa.get<0>().family() == Poco::Net::AddressFamily::IPv4)
                    {
                        Poco::Net::IPAddress ip = ipa.get<0>();
                        if (!ip.isLoopback() && !ip.isWildcard() && ip.isUnicast() && !ip.isBroadcast())
                        {
                            localIP = ip.toString();
                            // Retorna o primeiro endereço IP válido encontrado que não é um gateway
                            return localIP;
                        }
                    }
                }
            }
        }

        // Retorna o endereço loopback caso não encontre um endereço externo válido
        return localIP;
    }
};

// Função para carregar as configurações
void loadSettings(std::string &projectPath, int &port)
{
    // Define o caminho do arquivo de configuração
    std::string configPath = "config.json";
    std::ifstream configFile(configPath);

    // Verifica se o arquivo existe
    if (configFile.is_open())
    {
        json j;
        configFile >> j;

        // Carrega a pasta do projeto e a porta do arquivo JSON
        projectPath = j["projectPath"];
        port = j.value("port", 8080);

        configFile.close();
    }
    else
    {
        // Valores padrão caso o arquivo de configuração não exista
        projectPath = "";
        port = 8080;
    }
}

// Função para salvar as configurações
void saveSettings(const std::string &projectPath, int port)
{
    // Define o caminho do arquivo de configuração
    std::string configPath = "config.json";
    std::ofstream configFile(configPath);

    // Cria um objeto JSON e armazena a pasta do projeto e a porta
    json j;
    j["projectPath"] = projectPath;
    j["port"] = port;

    // Salva no arquivo
    configFile << j.dump(4); // Indentação de 4 espaços para melhor leitura

    configFile.close();
}

// Function to generate QRCode
GLuint generateQRCodeTexture(const std::string &data)
{
    cv::QRCodeEncoder::Params params;
    params.mode = cv::QRCodeEncoder::EncodeMode::MODE_BYTE; // Modo de codificação do QR Code
    cv::Ptr<cv::QRCodeEncoder> encoder = cv::QRCodeEncoder::create(params);

    std::vector<cv::Mat> qrcodes;

    // Gera o QR Code
    encoder->encodeStructuredAppend(data, qrcodes);
    if (qrcodes.empty())
        return 0; // Verifica se a geração foi bem-sucedida

    // Considerando apenas o primeiro QR Code para a textura
    cv::Mat qrCode = qrcodes.front();

    // Redimensiona o QR Code para uma resolução mais alta
    cv::Mat qrCodeHighRes;
    int newSize = 1024; // Nova dimensão quadrada para a imagem
    cv::resize(qrCode, qrCodeHighRes, cv::Size(newSize, newSize), 0, 0, cv::INTER_NEAREST);

    // Converte para RGBA
    cv::Mat qrCodeRGBA;
    cvtColor(qrCodeHighRes, qrCodeRGBA, cv::COLOR_BGR2RGBA);

    // Gera uma textura OpenGL a partir da imagem RGBA
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, qrCodeRGBA.cols, qrCodeRGBA.rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, qrCodeRGBA.data);

    return textureID;
}

// Function to load texture from image
ImageTexture LoadTextureFromImage(const char *imagePath)
{
    ImageTexture imgTexture = {0, 0, 0};
    int width, height, channels;
    unsigned char *data = stbi_load(imagePath, &width, &height, &channels, 0);
    if (data == nullptr)
    {
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

void TextAutoSizedAndCentered(const std::string &text, ImFont *font, bool useDisplaySize)
{
    ImGuiIO &io = ImGui::GetIO();

    // Define padding
    float paddingX = 20.0f; // Horizontal padding
    float paddingY = 20.0f; // Vertical padding

    ImVec2 baseSize; // Initialize base size
    ImVec2 basePos;  // Initialize base position

    if (useDisplaySize)
    {
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            // When viewports are enabled, use the main viewport's size and position
            ImGuiViewport *mainViewport = ImGui::GetMainViewport();
            baseSize = mainViewport->Size;
            basePos = mainViewport->Pos;
        }
        else
        {
            // If viewports are not enabled, use the display size and position (0, 0)
            baseSize = io.DisplaySize;
            basePos = ImVec2(0, 0);
        }
    }
    else
    {
        // When not using display size, use the current window's size and position
        baseSize = ImGui::GetWindowSize();
        basePos = ImGui::GetWindowPos();
    }

    // Ensure there's a minimum size for drawing text
    baseSize.x = (std::max)(baseSize.x, 1.0f); // Minimum width
    baseSize.y = (std::max)(baseSize.y, 1.0f); // Minimum height

    // Calculates available area considering padding
    float availableWidth = baseSize.x - 2 * paddingX;
    float availableHeight = baseSize.y - 2 * paddingY;

    // Finds the required width for the text and the number of lines
    float maxLineWidth = 0.0f;
    int lineCount = 0;
    std::istringstream stream(text);
    std::string line;
    while (std::getline(stream, line))
    {
        ImVec2 lineSize = font->CalcTextSizeA(font->FontSize, FLT_MAX, 0.0f, line.c_str());
        if (lineSize.x > maxLineWidth)
        {
            maxLineWidth = lineSize.x;
        }
        lineCount++;
    }

    // Adjusts the font size if the longest line is wider than the available space
    float scaleFactor = (maxLineWidth > availableWidth) ? (availableWidth / maxLineWidth) : 1.0f;
    float fontSize = font->FontSize * scaleFactor;

    // Ensures the text block fits vertically within the available height
    float totalTextHeight = fontSize * lineCount;
    if (totalTextHeight > availableHeight)
    {
        fontSize *= availableHeight / totalTextHeight;
    }

    // Prepares to draw the text
    ImDrawList *drawList = ImGui::GetForegroundDrawList();

    // Text and outline colors
    ImU32 textColor = IM_COL32(255, 255, 255, 255);
    ImU32 outlineColor = IM_COL32(0, 0, 0, 255);

    // Resets the stream to draw the text
    stream.clear();
    stream.seekg(0, std::ios::beg);

    // Calculates the starting y position to center the text block vertically
    float textPosY = basePos.y + paddingY + (availableHeight - fontSize * lineCount) / 2.0f;

    // Draws the text line by line
    while (std::getline(stream, line))
    {
        ImVec2 lineSize = font->CalcTextSizeA(fontSize, FLT_MAX, 0.0f, line.c_str());

        // Centers each line of text
        float textPosX = basePos.x + paddingX + (availableWidth - lineSize.x) / 2.0f;

        // Draws the outline
        float outlineThickness = 1.0f;
        for (int x = -outlineThickness; x <= outlineThickness; ++x)
        {
            for (int y = -outlineThickness; y <= outlineThickness; ++y)
            {
                if (x != 0 || y != 0)
                {
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

void windowCloseCallback(GLFWwindow *window)
{
    glfwDestroyWindow(window);
}

void windowKeyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    auto altF4 = (key == GLFW_KEY_W && mods == GLFW_MOD_SUPER);
    auto commandQ = (key == GLFW_KEY_F4 && mods == GLFW_MOD_ALT && action == GLFW_PRESS);

    if (altF4 || commandQ)
    {
        glfwDestroyWindow(window);
    }
}

int main()
{
    if (!glfwInit())
    {
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
    GLFWwindow *window = glfwCreateWindow(1024, 768, "Image Grid with ImGui", nullptr, nullptr);
    if (window == nullptr)
    {
        std::cerr << "Error creating GLFW window." << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Video window
    GLFWwindow *videoWindow = nullptr;

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
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    ImGui::StyleColorsDark();
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
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
    ImFont *fontMain = io.Fonts->AddFontFromFileTTF("fonts/OpenSans-Regular.ttf", 18, &fontConfig, io.Fonts->GetGlyphRangesDefault());

    if (!fontMain)
    {
        std::cerr << "Error while load font." << std::endl;
    }

    ImFont *fontMainTitle = io.Fonts->AddFontFromFileTTF("fonts/OpenSans-Bold.ttf", 18, &fontConfigBold, io.Fonts->GetGlyphRangesDefault());

    if (!fontMainTitle)
    {
        std::cerr << "Error while load font." << std::endl;
    }

    ImFont *fontPlayerText = io.Fonts->AddFontFromFileTTF("fonts/Poppins-Bold.ttf", 500, &fontConfig, io.Fonts->GetGlyphRangesDefault());

    if (!fontPlayerText)
    {
        std::cerr << "Error while load font." << std::endl;
    }

    // Load implementations for ImGui
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init();

    // Load settings
    std::string selectedProjectPath;
    int serverPort;

    loadSettings(selectedProjectPath, serverPort);

    // Load images from directory
    std::vector<ImageTexture> textures;
    std::string pathToImages;

    if (!selectedProjectPath.empty())
    {
        pathToImages = selectedProjectPath + "/images";
    }

    if (fs::is_directory(pathToImages))
    {
        for (const auto &entry : fs::directory_iterator(pathToImages))
        {
            if (entry.is_regular_file())
            {
                textures.push_back(LoadTextureFromImage(entry.path().string().c_str()));
            }
        }
    }

    // Open video file
    cv::VideoCapture video("videos/video1.mp4");
    if (!video.isOpened())
    {
        std::cerr << "Error opening video." << std::endl;
        return -1;
    }

    cv::Mat frame;
    GLuint videoTexture = 0;

    int videoWidth = 0;
    int videoHeight = 0;
    int videoStartFlags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoFocusOnAppearing;
    bool isVideoPlaying = true;

    GLuint selectedImageTexture = 0;
    int selectedImageWidth = 0, selectedImageHeight = 0;

    int monitorsCount;

    double fps = video.get(cv::CAP_PROP_FPS);
    auto frameDuration = std::chrono::milliseconds(static_cast<int>(1000 / fps));
    std::chrono::steady_clock::time_point lastFrameTime = std::chrono::steady_clock::now();

    bool starting = true;

    // server
    WebServer webServer;

    GLuint qrCodeTexture = 0; // ID da textura OpenGL para o QR Code
    std::string lastUrl;      // Última URL usada para gerar o QR Code

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // Get video frame
        auto currentTime = std::chrono::steady_clock::now();
        if (isVideoPlaying)
        {
            if (std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastFrameTime) >= frameDuration)
            {
                if (video.read(frame))
                {
                    // Convert BGR to RGBA
                    cv::Mat frameRGBA;
                    cv::cvtColor(frame, frameRGBA, cv::COLOR_BGR2RGBA);

                    // Update video texture
                    if (videoTexture == 0)
                    {
                        glGenTextures(1, &videoTexture);
                        glBindTexture(GL_TEXTURE_2D, videoTexture);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    }

                    glBindTexture(GL_TEXTURE_2D, videoTexture);
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, frameRGBA.cols, frameRGBA.rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, frameRGBA.data);

                    if (videoWidth == 0 || videoHeight == 0)
                    {
                        videoWidth = frame.cols;
                        videoHeight = frame.rows;
                    }

                    lastFrameTime = currentTime;
                }
                else
                {
                    // Restart video playback when reaching the end
                    video.set(cv::CAP_PROP_POS_FRAMES, 0);
                }
            }
        }

        // Main window
        glfwMakeContextCurrent(window);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Draw center text
        // TextAutoSizedAndCentered("DEUS ENVIOU\nSEU FILHO AMADO\nPRA PERDOAR\nPRA ME SALVAR", fontPlayerText, true);

        // Render tabs
        ImGui::PushFont(fontMain);

        ImGuiViewport *viewport = ImGui::GetMainViewport();

        ImGui::SetNextWindowPos(viewport->Pos);
        ImGui::SetNextWindowSize(viewport->Size);
        ImGui::SetNextWindowViewport(viewport->ID);

        ImGui::Begin("Control Panel", nullptr, ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);

        if (ImGui::BeginTabBar("TabBar"))
        {
            if (ImGui::BeginTabItem("Settings"))
            {
                // Project Folder Section
                ImGui::Dummy(ImVec2(0, 4));

                ImGui::PushFont(fontMainTitle);
                ImGui::Text("PROJECT FOLDER");
                ImGui::PopFont();

                ImGui::InputText("", selectedProjectPath.data(), selectedProjectPath.size(), ImGuiInputTextFlags_ReadOnly);

                ImGui::SameLine();

                if (ImGui::Button("Select Folder"))
                {
                    auto selectedFolder = pfd::select_folder("Select Project Folder", ".").result();

                    if (!selectedFolder.empty())
                    {
                        // strncpy(folderPathBuffer, selectedFolder.c_str(), sizeof(folderPathBuffer));
                        selectedProjectPath = selectedFolder;

                        // Limpa a lista de texturas existentes
                        textures.clear();
                        // Define o novo caminho para as imagens
                        pathToImages = selectedProjectPath + "/images";

                        // Recarrega as texturas
                        if (fs::is_directory(pathToImages))
                        {
                            for (const auto &entry : fs::directory_iterator(pathToImages))
                            {
                                if (entry.is_regular_file())
                                {
                                    textures.push_back(LoadTextureFromImage(entry.path().string().c_str()));
                                }
                            }
                        }
                    }
                }

                ImGui::Text("Hint: Select the folder where your project is located");

                ImGui::Dummy(ImVec2(0, 10));
                ImGui::Separator();
                ImGui::Dummy(ImVec2(0, 10));

                // Projector Controls Section
                ImGui::PushFont(fontMainTitle);
                ImGui::Text("PROJECTOR CONTROLS");
                ImGui::PopFont();

                if (ImGui::Button("Close Projector"))
                {
                    // Implement logic to close the projector
                }
                ImGui::SameLine();
                if (ImGui::Button("Black Screen"))
                {
                    // Implement logic to set the screen to black
                }
                ImGui::SameLine();
                if (ImGui::Button("Default Screen"))
                {
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
                ImGui::ColorEdit4("Text Color", (float *)&textColor, ImGuiColorEditFlags_NoInputs);

                static ImVec4 outlineColor = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
                ImGui::ColorEdit4("Outline Color", (float *)&outlineColor, ImGuiColorEditFlags_NoInputs);

                ImGui::Dummy(ImVec2(0, 10));
                ImGui::Separator();
                ImGui::Dummy(ImVec2(0, 10));

                // Controle de porta do servidor
                ImGui::PushFont(fontMainTitle);
                ImGui::Text("REMOTE CONTROL SETTINGS");
                ImGui::PopFont();

                ImGui::InputInt("Server Port", &serverPort);

                if (webServer.serverRunning)
                {
                    if (ImGui::Button("Stop Server"))
                    {
                        webServer.stop();
                    }

                    // Gera o QR Code apenas se a porta do servidor mudou
                    std::string currentUrl = "http://" + webServer.getLocalIPAddress() + ":" + std::to_string(serverPort) + "/rcontrol/?api_url=http://" + webServer.getLocalIPAddress() + ":" + std::to_string(serverPort) + "/api";
                    if (currentUrl != lastUrl)
                    {
                        // Se já tiver uma textura de QR Code, deleta a antiga
                        if (qrCodeTexture != 0)
                        {
                            glDeleteTextures(1, &qrCodeTexture);
                            qrCodeTexture = 0;
                        }

                        // Gera a nova textura de QR Code
                        qrCodeTexture = generateQRCodeTexture(currentUrl);
                        lastUrl = currentUrl;
                    }

                    // Exibe o QR Code se estiver disponível
                    if (qrCodeTexture != 0)
                    {
                        ImGui::Dummy(ImVec2(0, 10));

                        ImGui::Text("QR Code:");
                        ImGui::Image((void *)(intptr_t)qrCodeTexture, ImVec2(200, 200)); // Exibe o QR Code com tamanho 200x200
                    }
                }
                else
                {
                    if (ImGui::Button("Start Server"))
                    {
                        webServer.start(serverPort);
                    }
                }

                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Images"))
            {
                if (textures.empty())
                {
                    ImGui::SetCursorPosX((ImGui::GetContentRegionAvail().x - ImGui::CalcTextSize("No project selected").x) * 0.5f);
                    ImGui::SetCursorPosY((ImGui::GetContentRegionAvail().y - ImGui::CalcTextSize("No project selected").y) * 0.5f);
                    ImGui::Text("No images inside project folder");
                }
                else
                {
                    const ImVec2 cellSize(120.0f, 80.0f); // Fixed size for cells
                    float windowWidth = ImGui::GetContentRegionAvail().x;
                    const float paddingBetweenImages = 8.0f; // Define padding between images

                    // Calculate the total width used by each image, including padding
                    float totalCellWidth = cellSize.x + paddingBetweenImages;

                    // Adjust calculation for imagesPerRow to include padding, subtracting 1 padding since there's no padding after the last image in a row
                    int imagesPerRow = static_cast<int>((windowWidth + paddingBetweenImages) / totalCellWidth);

                    for (int i = 0; i < textures.size(); ++i)
                    {
                        // If it's not the first image and reached the end of the row, start a new row
                        if (i > 0 && i % imagesPerRow == 0)
                        {
                            ImGui::NewLine();
                        }

                        // Calculate the aspect ratio and size of the image to fit in the cell
                        float aspectRatio = static_cast<float>(textures[i].width) / textures[i].height;
                        ImVec2 imageSize = (aspectRatio > 1.0f) ? ImVec2(cellSize.x, cellSize.x / aspectRatio) : ImVec2(cellSize.y * aspectRatio, cellSize.y);

                        // Calculate padding to center the image in the cell
                        float paddingX = (cellSize.x - imageSize.x) / 2.0f;
                        float paddingY = (cellSize.y - imageSize.y) / 2.0f;

                        // Calculate position for the cell's top-left corner
                        ImVec2 cellPos = ImGui::GetCursorScreenPos();

                        // Adjust cursor position for padding and center the image
                        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + paddingX);
                        ImGui::SetCursorPosY(ImGui::GetCursorPosY() + paddingY);

                        // Draw the image
                        ImGui::Image((void *)(intptr_t)textures[i].textureID, imageSize);

                        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                        {
                            // Assume que esta é a condição para selecionar uma imagem após o clique duplo
                            isVideoPlaying = false;                       // Para a reprodução do vídeo
                            videoTexture = 0;                             // Reseta a textura do vídeo se necessário
                            selectedImageTexture = textures[i].textureID; // Atualiza a textura selecionada
                            selectedImageWidth = textures[i].width;       // Atualiza a largura da imagem selecionada
                            selectedImageHeight = textures[i].height;     // Atualiza a altura da imagem selecionada
                        }

                        // Draw rectangle around the cell
                        ImDrawList *drawList = ImGui::GetWindowDrawList();
                        drawList->AddRect(cellPos, ImVec2(cellPos.x + cellSize.x, cellPos.y + cellSize.y), IM_COL32(255, 255, 255, 255));

                        // If it's not the end of the row, continue on the same line
                        if ((i + 1) % imagesPerRow != 0 && (i + 1) < textures.size())
                        {
                            ImGui::SameLine();

                            // Reset cursor position to the left edge and prepare for the next image or row
                            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + paddingX);
                            ImGui::SetCursorPosY(ImGui::GetCursorPosY() - paddingY);
                        }
                    }
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();

        // Render image grid

        // Render video
        if (videoWindow)
        {
            glfwMakeContextCurrent(videoWindow);
            glfwPollEvents();
        }

        int videoFlags = videoStartFlags;
        ImVec2 videoWinPos;
        ImVec2 videoWinSize;
        GLFWmonitor **monitors = glfwGetMonitors(&monitorsCount);

        if (starting)
        {
            if (monitorsCount > 1 && io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                int monitorPosX, monitorPosY, monitorWidth, monitorHeight;
                glfwGetMonitorWorkarea(monitors[1], &monitorPosX, &monitorPosY, &monitorWidth, &monitorHeight);

                videoWinPos = ImVec2(monitorPosX, monitorPosY);
                videoWinSize = ImVec2(monitorWidth, monitorHeight);

                videoFlags = videoFlags | ImGuiWindowFlags_NoDecoration;
            }
            else
            {
                videoWinPos = ImVec2(ImGui::GetCursorPos().x + 50, ImGui::GetCursorPos().y + 300);
                videoWinSize = ImVec2(400, 200);
            }
        }
        else
        {
            if (monitorsCount > 1 && io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                int monitorPosX, monitorPosY, monitorWidth, monitorHeight;
                glfwGetMonitorWorkarea(monitors[1], &monitorPosX, &monitorPosY, &monitorWidth, &monitorHeight);

                videoWinPos = ImVec2(monitorPosX, monitorPosY);
                videoWinSize = ImVec2(monitorWidth, monitorHeight);

                videoFlags = videoFlags | ImGuiWindowFlags_NoDecoration;
            }
            else
            {
                videoWinPos = ImVec2(ImGui::GetCursorPos().x + 50, ImGui::GetCursorPos().y + 300);
                videoWinSize = ImVec2(400, 200);
            }
        }

        ImGui::Begin("Video Player", nullptr, videoFlags);
        ImGui::SetWindowPos(videoWinPos);
        ImGui::SetWindowSize(videoWinSize);

        TextAutoSizedAndCentered("DEUS ENVIOU\nSEU FILHO AMADO\nPRA PERDOAR\nPRA ME SALVAR", fontPlayerText, false);

        if (videoTexture != 0)
        {
            // Get total window dimensions from ImGui
            ImVec2 windowSize = ImGui::GetContentRegionAvail();

            // Calculate video aspect ratio
            float videoAspectRatio = (float)videoWidth / (float)videoHeight;
            float windowAspectRatio = windowSize.x / windowSize.y;

            // Calculate image size based on video and window aspect ratio
            ImVec2 imageSize;
            ImVec2 imagePos;

            if (videoAspectRatio > windowAspectRatio)
            {
                // Imagem é mais larga que a área disponível
                imageSize.x = windowSize.y * videoAspectRatio;    // Ajusta a largura baseado na altura da janela
                imageSize.y = windowSize.y;                       // Altura preenche a janela
                imagePos.x = (windowSize.x - imageSize.x) * 0.5f; // Centraliza horizontalmente
                imagePos.y = 0;                                   // Começa do topo da janela
            }
            else
            {
                // Imagem é mais alta que a área disponível
                imageSize.x = windowSize.x;                       // Largura preenche a janela
                imageSize.y = windowSize.x / videoAspectRatio;    // Ajusta a altura baseado na largura da janela
                imagePos.x = 0;                                   // Começa da lateral esquerda da janela
                imagePos.y = (windowSize.y - imageSize.y) * 0.5f; // Centraliza verticalmente
            }

            // Apply calculated position
            ImGui::SetCursorPos(imagePos);

            // Render video image with adjusted size and position
            ImGui::Image((void *)(intptr_t)videoTexture, imageSize);
        }
        else if (selectedImageTexture != 0)
        {
            ImVec2 windowSize = ImGui::GetContentRegionAvail(); // Tamanho disponível na janela

            // Presumindo que você tem acesso às dimensões originais da imagem
            float imageAspectRatio = static_cast<float>(selectedImageWidth) / selectedImageHeight;
            float windowAspectRatio = windowSize.x / windowSize.y;

            ImVec2 imageSize;
            ImVec2 imagePos;

            if (imageAspectRatio > windowAspectRatio)
            {
                // Imagem é mais larga que a área disponível
                imageSize.x = windowSize.y * imageAspectRatio;    // Ajusta a largura baseado na altura da janela
                imageSize.y = windowSize.y;                       // Altura preenche a janela
                imagePos.x = (windowSize.x - imageSize.x) * 0.5f; // Centraliza horizontalmente
                imagePos.y = 0;                                   // Começa do topo da janela
            }
            else
            {
                // Imagem é mais alta que a área disponível
                imageSize.x = windowSize.x;                       // Largura preenche a janela
                imageSize.y = windowSize.x / imageAspectRatio;    // Ajusta a altura baseado na largura da janela
                imagePos.x = 0;                                   // Começa da lateral esquerda da janela
                imagePos.y = (windowSize.y - imageSize.y) * 0.5f; // Centraliza verticalmente
            }

            // Aplica a posição calculada
            ImGui::SetCursorPos(imagePos);

            // Renderiza a imagem com o tamanho e posição ajustados
            ImGui::Image((void *)(intptr_t)selectedImageTexture, imageSize);
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

    // Antes de fechar o servidor e terminar a aplicação
    saveSettings(selectedProjectPath, serverPort);

    // stop server
    webServer.stop();

    // Cleanup
    if (videoTexture != 0)
    {
        glDeleteTextures(1, &videoTexture);
    }

    for (auto &texture : textures)
    {
        glDeleteTextures(1, &texture.textureID);
    }

    if (selectedImageTexture != 0)
    {
        glDeleteTextures(1, &selectedImageTexture);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (videoWindow)
    {
        glfwDestroyWindow(videoWindow);
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
