///////////////////////////////////////////////////////////////////////
// Provides the framework for graphics projects, mostly just GLFW
// initialization and main loop.
////////////////////////////////////////////////////////////////////////

#include "framework.h"


static void error_callback(int error, const char* msg) {
    fputs(msg, stderr);
}

////////////////////////////////////////////////////////////////////////
// Do the OpenGL/GLFW setup and then enter the interactive loop.
int main(int argc, char** argv) {
    //_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    //_CrtSetBreakAlloc(169);
    glfwSetErrorCallback(error_callback);

    // Initialize the OpenGL bindings
    // glbinding::Binding::initialize(false);

    // Initialize glfw open a window
    if (!glfwInit())  exit(EXIT_FAILURE);

    //glfwWindowHint(GLFW_RESIZABLE, 1);
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    //glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        Scene scene;
        scene.window = glfwCreateWindow(1920, 1080, "Graphics Framework", NULL, NULL);
        if (!scene.window) { glfwTerminate();  exit(-1); }

        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;

        glfwSetWindowUserPointer(scene.window, &scene);

        fflush(stdout);

        // Initialize interaction and the scene to be drawn.

        // Enter the event loop.
        try {
            printf_s("hi\n");
            InitInteraction(scene.window);
            printf_s("hi\n");
            ImGui_ImplGlfw_InitForOther(scene.window, true);
            printf_s("hi\n");
            scene.InitializeScene();
            while (!glfwWindowShouldClose(scene.window)) {

                glfwPollEvents();
                scene.DrawScene();
                scene.DrawMenu();
                scene.EndFrame();
            }
        }
        catch (std::exception& e) {
            printf("%s", e.what());
        }
    }
    //ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    //_CrtDumpMemoryLeaks();
}
