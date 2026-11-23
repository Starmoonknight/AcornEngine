#include "InputSystem.h"


// Standard library 
#include <cassert>




namespace Input
{
    
    InputSystem::InputSystem(GLFWwindow* window)
        :window_(window)
    {
        assert(window_ != nullptr);
        
        // This implementation uses the window's one user-pointer slot.
        assert(glfwGetWindowUserPointer(window_) == nullptr);
        
        glfwSetKeyCallback(window_, OnKey);
        glfwSetMouseButtonCallback(window_, OnMouseButton);
        glfwSetScrollCallback(window_, OnScroll);
        glfwSetWindowUserPointer(window_, this);
    }

    InputSystem::~InputSystem()
    {
        // The window must still exist here.
        glfwSetKeyCallback(window_, nullptr);
        glfwSetMouseButtonCallback(window_, nullptr);
        glfwSetScrollCallback(window_, nullptr);
        glfwSetWindowUserPointer(window_, nullptr);
    }
    
    
    void InputSystem::ApplyButtonEvent(ButtonState& state, int action)
    {
        if (action == GLFW_PRESS)
        {
            state.pressed = true;
            state.held = true;
        }
        else if (action == GLFW_RELEASE)
        {
            state.released = true;
            state.held = false;
        }

        // Ignore GLFW_REPEAT. Held remains true until release.
    }

    // some parameters are unused, but are needed because GLFW expects this callback signature.
    void InputSystem::OnKey(GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        auto* input = static_cast<InputSystem*>(
            glfwGetWindowUserPointer(window));

        // GLFW_KEY_UNKNOWN is negative and must not index the array.
        if (input && key >= 0 && key < KeyCount)
            ApplyButtonEvent(input->pendingKeys_[key], action);
    }
    
    
    void InputSystem::OnMouseButton(GLFWwindow* window, int button, int action, int mods)
    {
        auto* input = static_cast<InputSystem*>(
            glfwGetWindowUserPointer(window));

        if (input && button >= 0 && button < MouseButtonCount)
            ApplyButtonEvent(input->pendingButtons_[button], action);
    }

    
    void InputSystem::OnScroll(GLFWwindow* window, double xOffset, double yOffset)
    {
        auto* input = static_cast<InputSystem*>(
            glfwGetWindowUserPointer(window));

        if (input)
        {
            input->pendingScrollX_ += xOffset;
            input->pendingScrollY_ += yOffset;
        }
    }

    
    ButtonState InputSystem::Key(int key) const
    {
        if (key < 0 || key >= KeyCount)
            return {};

        return keys_[key];
    }

    
    ButtonState InputSystem::MouseButton(int button) const
    {
        if (button < 0 || button >= MouseButtonCount)
            return {};

        return buttons_[button];
    }
    

    void InputSystem::ResetMouseDelta()
    {
        haveMouseBaseline_ = false;
        mouse_.deltaX = 0.0;
        mouse_.deltaY = 0.0; 
    }
    
    
    void InputSystem::Update()
    {
        glfwPollEvents();

        keys_ = pendingKeys_;
        buttons_ = pendingButtons_;

        // Clear only transient events; down states persist.
        for (auto& key : pendingKeys_)
        {
            key.pressed = false;
            key.released = false;
        }

        for (auto& button : pendingButtons_)
        {
            button.pressed = false;
            button.released = false;
        }

        mouse_.scrollX = pendingScrollX_;
        mouse_.scrollY = pendingScrollY_;
        pendingScrollX_ = 0.0;
        pendingScrollY_ = 0.0;

        glfwGetCursorPos(window_, &mouse_.x, &mouse_.y);

        const bool focused =
            glfwGetWindowAttrib(window_, GLFW_FOCUSED) == GLFW_TRUE;

        mouse_.deltaX = 0.0;
        mouse_.deltaY = 0.0;

        if (focused && haveMouseBaseline_)
        {
            mouse_.deltaX = mouse_.x - previousX_;
            mouse_.deltaY = mouse_.y - previousY_;
        }

        previousX_ = mouse_.x;
        previousY_ = mouse_.y;
        haveMouseBaseline_ = focused;
    }

    
    
}
