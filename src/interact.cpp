////////////////////////////////////////////////////////////////////////
// All keyboard, mouse, and other interactions are implemented here.
// The single entry point, InitInteraction, sets up GLFW callbacks for
// various events that an interactive graphics program needs to
// handle.
//

#include "framework.h"
#include "scene.h"

// Some globals used for mouse handling.
double mouseX, mouseY;
bool shifted = false;
bool leftDown = false;
bool middleDown = false;
bool rightDown = false;
bool control = false;

////////////////////////////////////////////////////////////////////////
// Function called to exit
void Quit(void* clientData) {
    //glfwSetWindowShouldClose(scene.window, 1);
}

std::string ACTION[3] = { "Release", "Press", "Repeat" };

////////////////////////////////////////////////////////////////////////
// Called for keyboard actions.

void Keyboard(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (ImGui::GetIO().WantCaptureKeyboard)
        return;

    if (action == GLFW_REPEAT)
        return; // Because keyboard autorepeat is evil.

    printf("Keyboard %c(%d);  S%d %s M%d\n", key, key, scancode, ACTION[action].c_str(), mods);
    fflush(stdout);

    // Track SHIFT/NO-SHIFT transitions. (The mods parameter should do this, but doesn't.)
    if (key == GLFW_KEY_LEFT_SHIFT || key == GLFW_KEY_RIGHT_SHIFT)
        shifted = !shifted;
    if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL)
        control = !control;

    // @@ Catch any key DOWN-transition you'd like in the following
    // switch statement.  Record any change of state in variables in
    // the scene object.

    Scene* scene = static_cast<Scene*>(glfwGetWindowUserPointer(window));
    if (action == GLFW_PRESS) {
        switch (key) {

        case GLFW_KEY_TAB:
            scene->nav = !scene->nav;
            break;

        case GLFW_KEY_W:
            scene->w_down = true;
            break;
        case GLFW_KEY_S:
            scene->s_down = true;
            break;
        case GLFW_KEY_A:
            scene->a_down = true;
            break;
        case GLFW_KEY_D:
            scene->d_down = true;
            break;

        case GLFW_KEY_0:
        case GLFW_KEY_1:
        case GLFW_KEY_2:
        case GLFW_KEY_3:
        case GLFW_KEY_4:
        case GLFW_KEY_5:
        case GLFW_KEY_6:
        case GLFW_KEY_7:
        case GLFW_KEY_8:
        case GLFW_KEY_9:
            break;
        case GLFW_KEY_ESCAPE:
        case GLFW_KEY_Q: // Escape and 'q' keys quit the application
            exit(0);
        }
    }

    else if (action == GLFW_RELEASE) {

        switch (key) {
        case GLFW_KEY_W:
            scene->w_down = false;
            break;
        case GLFW_KEY_S:
            scene->s_down = false;
            break;
        case GLFW_KEY_A:
            scene->a_down = false;
            break;
        case GLFW_KEY_D:
            scene->d_down = false;
            break;
        }
    }

    // @@ Catch any key UP-transitions you want here.  Record any
    // change of state in variables in the scene object.
    fflush(stdout);
}

////////////////////////////////////////////////////////////////////////
// Called when a mouse button changes state.
void MouseButton(GLFWwindow* window, int button, int action, int mods) {

    if (ImGui::GetIO().WantCaptureMouse)
        return;

    glfwGetCursorPos(window, &mouseX, &mouseY);
    printf("MouseButton %d %d %d %f %f\n", button, action, mods, mouseX, mouseY);

    // @@ Catch any mouse button UP-or-DOWN-transitions you want here.
    // Record any change of state in variables in the scene object.

    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        leftDown = (action == GLFW_PRESS);
    }

    else if (button == GLFW_MOUSE_BUTTON_MIDDLE) {
        middleDown = (action == GLFW_PRESS);
    }

    else if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        rightDown = (action == GLFW_PRESS);
    }
}

////////////////////////////////////////////////////////////////////////
// Called by GLFW when a mouse moves (while a button is down)
void MouseMotion(GLFWwindow* window, double x, double y) {
    if (ImGui::GetIO().WantCaptureMouse)
        return;

    // @@ Catch any mouse movement that occurs while any button is
    // down.  It is not reported here *which* button is down, but you
    // should have recorded the DOWN-transition in a previous call to
    // MouseButton.

    // @@ The x,y parameters to this function record the position of
    // the mouse.  Usually you want to know only how much it has moved
    // since the last call.  Hence the following two lines (and this
    // procedure's last two lines) calculate the change in the mouse
    // position.

    // Calculate the change in the mouse position
    int dx = static_cast<int>(x - mouseX);
    int dy = static_cast<int>(y - mouseY);

    Scene* scene = static_cast<Scene*>(glfwGetWindowUserPointer(window));
    if (leftDown) {
        using namespace DirectX;
        using namespace DirectX::SimpleMath;
        // Rotate eye position
        
        scene->cameraForward = Vector3::TransformNormal(scene->cameraForward, 
            Matrix::CreateFromAxisAngle(scene->right, XMConvertToRadians(-dy / 6.0f)));
        scene->cameraForward.Normalize();

        scene->right = scene->cameraForward.Cross(Vector3::Up);
        scene->right.Normalize();

        scene->up = scene->right.Cross(scene->cameraForward);
        scene->up.Normalize();

        scene->cameraForward = Vector3::TransformNormal(scene->cameraForward,
            Matrix::CreateFromAxisAngle(scene->up, XMConvertToRadians(-dx / 6.0f)));
        scene->cameraForward.Normalize();

        scene->right = scene->cameraForward.Cross(Vector3::Up);
        scene->right.Normalize();

        scene->up = scene->right.Cross(scene->cameraForward);
        scene->up.Normalize();
    }

    if (middleDown) {}

    if (rightDown) {
        scene->cameraPos.x += dx / 40.0f;
        scene->cameraPos.y -= dy / 40.0f;
    }

    // Record this position
    mouseX = x;
    mouseY = y;
}

void Scroll(GLFWwindow* window, double x, double y) {
    if (ImGui::GetIO().WantCaptureMouse)
        return;

    printf("Scroll %f %f\n", x, y);

    Scene* scene = static_cast<Scene*>(glfwGetWindowUserPointer(window));
    // Figure out the mouse action, and handle accordingly
    // @@ Please don't disable this shifted-scroll-wheel code.
    //if (y > 0.0 && shifted) { // Scroll light in
    //    scene->lightDist = pow(scene->lightDist, 1.0f / 1.02f);
    //}

    //// @@ Please don't disable this shifted-scroll-wheel code.
    //else if (y < 0.0 && shifted) { // Scroll light out
    //    scene->lightDist = pow(scene->lightDist, 1.02f);
    //}

    if (control) { // Scroll light out
        float a = y < 0.0f ? 1.03f : 1.0f / 1.03f;
        scene->ry /= a;
        scene->cameraPos.z *= a;
        printf("ry: %f\n", scene->ry);
    }

    else if (y > 0.0) {
        scene->cameraPos.z = pow(scene->cameraPos.z, 1.0f / 1.02f);
    }

    else if (y < 0.0) {
        scene->cameraPos.z = pow(scene->cameraPos.z, 1.02f);
    }
}

void InitInteraction(GLFWwindow* _pWindow) {
    glfwSetKeyCallback(_pWindow, Keyboard);
    glfwSetMouseButtonCallback(_pWindow, MouseButton);
    glfwSetCursorPosCallback(_pWindow, MouseMotion);
    glfwSetScrollCallback(_pWindow, Scroll);
}
