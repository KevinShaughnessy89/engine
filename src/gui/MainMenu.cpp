#include "MainMenu.hpp"

#include "core/PrecompiledHeader.hpp"

GameState MainMenu::renderMainMenu() {
    ImGui::GetIO().MouseDrawCursor = true;

    ImGui::SetNextWindowBgAlpha(1.0f);  // Fully opaque

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);

    ImGui::Begin("Main Menu", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus);

    ImGui::Text("Engine");

    // Center the button using spacing
    ImVec2 newWindowSize = ImGui::GetWindowSize();  // size of current ImGui window content area

    ImVec2 buttonSize(200, 50);
    ImVec2 enterButtonCenter = newWindowSize;
    enterButtonCenter.x = newWindowSize.x / 2.0f - 0.50f * buttonSize.x;
    enterButtonCenter.y = newWindowSize.y / 2.0f - 0.50f * buttonSize.y;

    ImGui::SetCursorPos(enterButtonCenter);

    GameState returnType = GameState::MAIN_MENU;

    if (ImGui::Button("Enter Game", buttonSize)) {
        ImGui::GetIO().MouseDrawCursor = false;
        returnType = GameState::LOADING;
    }

    float padding = 10.0f;

    ImGui::SetCursorPos(ImVec2(enterButtonCenter.x, enterButtonCenter.y + buttonSize.y + padding));

    if (ImGui::Button("Exit Game", buttonSize)) {
        returnType = GameState::QUIT;
    }

    ImGui::End();

    return returnType;
}

void MainMenu::runEntrySplash() {
}

GameState MainMenu::renderLoadingScreen(float loadingProgress) {
    float padding = 40.0f;
    float progressBarHeight = 20.0f;

    ImGui::GetIO().MouseDrawCursor = false;
    ImVec2 windowSize = ImGui::GetIO().DisplaySize;

    ImGui::SetNextWindowBgAlpha(1.0f);  // Fully opaque

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);

    ImGui::Begin("Loading", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoBackground);

    ImGui::SetCursorPosY(windowSize.y - progressBarHeight - padding);

    ImGui::Text("Loading world...");
    ImGui::ProgressBar(loadingProgress, ImVec2(-1.0f, 0), NULL);

    ImGui::End();

    if (loadingProgress == 1.f)
        return GameState::RUNNING;
    else
        return GameState::LOADING;
}

void MainMenu::init(GLFWwindow* window) {
    ImGui::CreateContext();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 420");

    ImGuiIO& io = ImGui::GetIO();
    ImGui::StyleColorsDark();
    io.Fonts->AddFontFromFileTTF("C:/dev/Projects/engine/assets/gui/Inter_18pt-Regular.ttf", 18.0f);
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.WindowBorderSize = 0.0f;
    style.WindowPadding = ImVec2(12, 12);
    style.ItemSpacing = ImVec2(8, 6);

    this->window = window;
}

void MainMenu::close() {
    std::cout << "CLOSE CALLED" << std::endl;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}
