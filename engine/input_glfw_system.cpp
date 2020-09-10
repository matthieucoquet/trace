#include "input_glfw_system.hpp"
#include "core/scene.hpp"
#include <imgui.h>
#include "vulkan/vk_common.hpp"

#include <GLFW/glfw3.h>
#ifdef _WIN32
#undef APIENTRY
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>   // for glfwGetWin32Window
#endif

static bool mouse_just_pressed[ImGuiMouseButton_COUNT] = {};
static bool installed_callbacks = false;

// Chain GLFW callbacks: our callbacks will call the user's previously installed callbacks, if any.
static GLFWmousebuttonfun prev_user_callback_mousebutton = nullptr;
static GLFWscrollfun prev_user_callback_scroll = nullptr;
static GLFWkeyfun prev_user_callback_key = nullptr;
static GLFWcharfun prev_user_callback_char = nullptr;

static const char* imgui_get_clipboard_text(void* user_data)
{
    return glfwGetClipboardString(reinterpret_cast<Input_glfw_system*>(user_data)->window);
}

static void imgui_set_clipboard_text(void* user_data, const char* text)
{
    glfwSetClipboardString(reinterpret_cast<Input_glfw_system*>(user_data)->window, text);
}

void imgui_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (prev_user_callback_key)
        prev_user_callback_key(window, key, scancode, action, mods);

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    if (action == GLFW_PRESS)
        io.KeysDown[key] = true;
    if (action == GLFW_RELEASE)
        io.KeysDown[key] = false;

    // Modifiers are not reliable across systems
    io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
    io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
    io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
#ifdef _WIN32
    io.KeySuper = false;
#else
    io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
#endif
}

void imgui_char_callback(GLFWwindow* window, unsigned int c)
{
    if (prev_user_callback_char)
        prev_user_callback_char(window, c);
    ImGuiIO& io = ImGui::GetIO();
    io.AddInputCharacter(c);
}

void imgui_mouse_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (prev_user_callback_mousebutton)
        prev_user_callback_mousebutton(window, button, action, mods);

    if (action == GLFW_PRESS && button >= 0 && button < IM_ARRAYSIZE(mouse_just_pressed))
        mouse_just_pressed[button] = true;
}

void imgui_scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    if (prev_user_callback_scroll)
        prev_user_callback_scroll(window, xoffset, yoffset);

    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheelH += (float)xoffset;
    io.MouseWheel += (float)yoffset;
}

Input_glfw_system::Input_glfw_system(GLFWwindow* glfw_window) :
    window(glfw_window)
{
    ImGuiIO& io = ImGui::GetIO();
    //io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    //io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
    io.BackendPlatformName = "imgui_impl_glwf";

    // Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array.
    io.KeyMap[ImGuiKey_Tab] = GLFW_KEY_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = GLFW_KEY_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = GLFW_KEY_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = GLFW_KEY_UP;
    io.KeyMap[ImGuiKey_DownArrow] = GLFW_KEY_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = GLFW_KEY_PAGE_UP;
    io.KeyMap[ImGuiKey_PageDown] = GLFW_KEY_PAGE_DOWN;
    io.KeyMap[ImGuiKey_Home] = GLFW_KEY_HOME;
    io.KeyMap[ImGuiKey_End] = GLFW_KEY_END;
    io.KeyMap[ImGuiKey_Insert] = GLFW_KEY_INSERT;
    io.KeyMap[ImGuiKey_Delete] = GLFW_KEY_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = GLFW_KEY_BACKSPACE;
    io.KeyMap[ImGuiKey_Space] = GLFW_KEY_SPACE;
    io.KeyMap[ImGuiKey_Enter] = GLFW_KEY_ENTER;
    io.KeyMap[ImGuiKey_Escape] = GLFW_KEY_ESCAPE;
    io.KeyMap[ImGuiKey_KeyPadEnter] = GLFW_KEY_KP_ENTER;
    io.KeyMap[ImGuiKey_A] = GLFW_KEY_A;
    io.KeyMap[ImGuiKey_C] = GLFW_KEY_C;
    io.KeyMap[ImGuiKey_V] = GLFW_KEY_V;
    io.KeyMap[ImGuiKey_X] = GLFW_KEY_X;
    io.KeyMap[ImGuiKey_Y] = GLFW_KEY_Y;
    io.KeyMap[ImGuiKey_Z] = GLFW_KEY_Z;

    io.SetClipboardTextFn = imgui_set_clipboard_text;
    io.GetClipboardTextFn = imgui_get_clipboard_text;
    io.ClipboardUserData = this;
#if defined(_WIN32)
    io.ImeWindowHandle = (void*)glfwGetWin32Window(window);
#endif

    io.MouseDrawCursor = true;
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    assert(glfwRawMouseMotionSupported());
    glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

    installed_callbacks = true;
    prev_user_callback_mousebutton = glfwSetMouseButtonCallback(window, imgui_mouse_callback);
    prev_user_callback_scroll = glfwSetScrollCallback(window, imgui_scroll_callback);
    prev_user_callback_key = glfwSetKeyCallback(window, imgui_key_callback);
    prev_user_callback_char = glfwSetCharCallback(window, imgui_char_callback);
}

Input_glfw_system::~Input_glfw_system()
{
    glfwSetMouseButtonCallback(window, prev_user_callback_mousebutton);
    glfwSetScrollCallback(window, prev_user_callback_scroll);
    glfwSetKeyCallback(window, prev_user_callback_key);
    glfwSetCharCallback(window, prev_user_callback_char);
}

void Input_glfw_system::step(Scene& scene)
{
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.Fonts->IsBuilt() && "Font atlas not built! It is generally built by the renderer back-end. Missing call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

    // Setup time step
    // TODO use openxr time function instead
    double current_time = glfwGetTime();
    io.DeltaTime = m_time > 0.0 ? (float)(current_time - m_time) : (float)(1.0f / 90.0f);
    m_time = current_time;

    update_mouse_pos_and_buttons(scene);

    // Update game controllers (if enabled and available)
    //ImGui_ImplGlfw_UpdateGamepads();
}

void Input_glfw_system::update_mouse_pos_and_buttons(Scene& scene)
{
    // Update buttons
    ImGuiIO& io = ImGui::GetIO();
    for (int i = 0; i < IM_ARRAYSIZE(io.MouseDown); i++)
    {
        // If a mouse press event came, always pass it as "mouse held this frame", so we don't miss click-release events that are shorter than 1 frame.
        bool pressed = mouse_just_pressed[i] || glfwGetMouseButton(window, i) != 0;
        mouse_just_pressed[i] = false;
        if (pressed)
            scene.mouse_control = true;
        if (scene.mouse_control)
            io.MouseDown[i] = pressed;
    }

    if (scene.mouse_control)
    {
        double mouse_x, mouse_y;
        glfwGetCursorPos(window, &mouse_x, &mouse_y);
        float updated_mouse_x = std::clamp(static_cast<float>(mouse_x), 0.0f, io.DisplaySize.x);
        float updated_mouse_y = std::clamp(static_cast<float>(mouse_y), 0.0f, io.DisplaySize.y);
        if (updated_mouse_x != mouse_x || updated_mouse_y != mouse_y) {
            glfwSetCursorPos(window, updated_mouse_x, updated_mouse_y);
        }
        io.MousePos = ImVec2(updated_mouse_x, updated_mouse_y);
    }
}
/*
static void ImGui_ImplGlfw_UpdateGamepads()
{
    ImGuiIO& io = ImGui::GetIO();
    memset(io.NavInputs, 0, sizeof(io.NavInputs));
    if ((io.ConfigFlags & ImGuiConfigFlags_NavEnableGamepad) == 0)
        return;

    // Update gamepad inputs
    #define MAP_BUTTON(NAV_NO, BUTTON_NO)       { if (buttons_count > BUTTON_NO && buttons[BUTTON_NO] == GLFW_PRESS) io.NavInputs[NAV_NO] = 1.0f; }
    #define MAP_ANALOG(NAV_NO, AXIS_NO, V0, V1) { float v = (axes_count > AXIS_NO) ? axes[AXIS_NO] : V0; v = (v - V0) / (V1 - V0); if (v > 1.0f) v = 1.0f; if (io.NavInputs[NAV_NO] < v) io.NavInputs[NAV_NO] = v; }
    int axes_count = 0, buttons_count = 0;
    const float* axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1, &axes_count);
    const unsigned char* buttons = glfwGetJoystickButtons(GLFW_JOYSTICK_1, &buttons_count);
    MAP_BUTTON(ImGuiNavInput_Activate,   0);     // Cross / A
    MAP_BUTTON(ImGuiNavInput_Cancel,     1);     // Circle / B
    MAP_BUTTON(ImGuiNavInput_Menu,       2);     // Square / X
    MAP_BUTTON(ImGuiNavInput_Input,      3);     // Triangle / Y
    MAP_BUTTON(ImGuiNavInput_DpadLeft,   13);    // D-Pad Left
    MAP_BUTTON(ImGuiNavInput_DpadRight,  11);    // D-Pad Right
    MAP_BUTTON(ImGuiNavInput_DpadUp,     10);    // D-Pad Up
    MAP_BUTTON(ImGuiNavInput_DpadDown,   12);    // D-Pad Down
    MAP_BUTTON(ImGuiNavInput_FocusPrev,  4);     // L1 / LB
    MAP_BUTTON(ImGuiNavInput_FocusNext,  5);     // R1 / RB
    MAP_BUTTON(ImGuiNavInput_TweakSlow,  4);     // L1 / LB
    MAP_BUTTON(ImGuiNavInput_TweakFast,  5);     // R1 / RB
    MAP_ANALOG(ImGuiNavInput_LStickLeft, 0,  -0.3f,  -0.9f);
    MAP_ANALOG(ImGuiNavInput_LStickRight,0,  +0.3f,  +0.9f);
    MAP_ANALOG(ImGuiNavInput_LStickUp,   1,  +0.3f,  +0.9f);
    MAP_ANALOG(ImGuiNavInput_LStickDown, 1,  -0.3f,  -0.9f);
    #undef MAP_BUTTON
    #undef MAP_ANALOG
    if (axes_count > 0 && buttons_count > 0)
        io.BackendFlags |= ImGuiBackendFlags_HasGamepad;
    else
        io.BackendFlags &= ~ImGuiBackendFlags_HasGamepad;
}*/
