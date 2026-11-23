#pragma once


// Standard library 
#include <array>

// Third-party libraries

// Prevent GLFW from including OpenGL headers.
// OpenGL functions elsewhere still come from glad.
#ifndef GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_NONE
#endif
#include <GLFW/glfw3.h>




// Note: 
//    Replace the backend later, for now using GLFW_KEY_A.
//    Look into an observer pattern event based input system later. 
//    Keeping the backend once set up but allow events to build InputContext




namespace Input
{
  
  
  struct ButtonState
  {
    bool pressed = false;
    bool held = false;
    bool released = false;
  };

  struct MouseState
  {
    // Window content coordinates, not framebuffer pixels.
    double x = 0.0;
    double y = 0.0;

    double deltaX = 0.0;
    double deltaY = 0.0;

    double scrollX = 0.0;
    double scrollY = 0.0;
  };
  
  
  // One instance attached to one GLFW window.
  //
  // Owns input state, not the window. Destroy before destroying the window.
  // Reserves the window user pointer and the key/mouse-button/scroll callbacks.
  // Do not replace those callbacks or that user pointer while attached.
  //
  // Call Update once per frame, before reading input.
  // Queries do not consume input: every reader sees the same snapshot.
  struct InputSystem
  {
  public:
    explicit InputSystem(GLFWwindow* window);
    ~InputSystem();
    
    InputSystem(const InputSystem&) = delete;
    InputSystem& operator=(const InputSystem&) = delete;
    InputSystem(InputSystem&&) = delete;
    InputSystem& operator=(InputSystem&&) = delete;
    
    // Processes GLFW events and publishes this frame's input snapshot.
    void Update();

    ButtonState Key(int key) const;
    ButtonState MouseButton(int button) const;
    const MouseState& Mouse() const { return mouse_; }

    // Use after changing cursor mode or deliberately repositioning it.
    // Prevents that change from becoming a large look movement.
    void ResetMouseDelta();
    
    
  private:
    static constexpr int KeyCount = GLFW_KEY_LAST + 1;
    static constexpr int MouseButtonCount = GLFW_MOUSE_BUTTON_LAST + 1;
    
    GLFWwindow* window_;
    
    // Callbacks write pending state. Gameplay reads published state.
    std::array<ButtonState, KeyCount> pendingKeys_{};
    std::array<ButtonState, KeyCount> keys_{};

    std::array<ButtonState, MouseButtonCount> pendingButtons_{};
    std::array<ButtonState, MouseButtonCount> buttons_{};

    double pendingScrollX_ = 0.0;
    double pendingScrollY_ = 0.0;

    MouseState mouse_{};
    double previousX_ = 0.0;
    double previousY_ = 0.0;
    bool haveMouseBaseline_ = false;
    
    
    static void ApplyButtonEvent(ButtonState& state, int action);

    static void OnKey(
        GLFWwindow* window, int key, int scancode, int action, int mods);

    static void OnMouseButton(
        GLFWwindow* window, int button, int action, int mods);

    static void OnScroll(
        GLFWwindow* window, double xOffset, double yOffset);
    
  };
  
  
  
  
  
  
  
  
  
  
  
}


