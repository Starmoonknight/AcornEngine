#pragma once


// Project headers
#include "InputSystem.h"

// Standard library 
#include <array>
#include <optional>



namespace Input
{
    
    struct PlayerBindings
    {
        int forward;
        int backward;
        int left;
        int right;
        int up;
        int down;

        int lookLeft;
        int lookRight;
        int lookUp;
        int lookDown;
    };

    
    struct PlayerInput
    {
        float forward = 0.0f;
        float right = 0.0f;
        float up = 0.0f;

        // Positive means look right/up.
        float lookRight = 0.0f;
        float lookUp = 0.0f;
    };
    
    
    inline float ReadAxis(
    const InputSystem& input, int negativeKey, int positiveKey)
    {
        return static_cast<float>(input.Key(positiveKey).held)
             - static_cast<float>(input.Key(negativeKey).held);
    }
    

    inline PlayerInput ReadPlayerInput(
        const InputSystem& input, const PlayerBindings& bindings)
    {
        PlayerInput result;

        result.forward = ReadAxis(
            input, bindings.backward, bindings.forward);

        result.right = ReadAxis(
            input, bindings.left, bindings.right);

        result.up = ReadAxis(
            input, bindings.down, bindings.up);

        result.lookRight = ReadAxis(
            input, bindings.lookLeft, bindings.lookRight);

        result.lookUp = ReadAxis(
            input, bindings.lookDown, bindings.lookUp);

        return result;
    }
    
    
    
}


namespace Demo
{
    
    const Input::PlayerBindings player1Bindings{
        GLFW_KEY_W, GLFW_KEY_S,
        GLFW_KEY_A, GLFW_KEY_D,
        GLFW_KEY_E, GLFW_KEY_Q, 
        GLFW_KEY_F, GLFW_KEY_H, 
        GLFW_KEY_T, GLFW_KEY_G
    };
    
    
    const Input::PlayerBindings player2Bindings{
        GLFW_KEY_I, GLFW_KEY_K,
        GLFW_KEY_J, GLFW_KEY_L,
        GLFW_KEY_O, GLFW_KEY_U, 
        GLFW_KEY_LEFT, GLFW_KEY_RIGHT, 
        GLFW_KEY_UP, GLFW_KEY_DOWN
    };
    
    
    struct FrameInput
    {
        bool quit = false;
        bool toggleWireframe = false;
        bool toggleCulling = false;
        bool toggleProjection = false;
        bool toggleBackground = false;
        bool toggleMultiplayer = false;
        
        bool toggleCarousel = false;

        std::optional<std::size_t> requestedScene;
        std::array<Input::PlayerInput, 2> players{};
    };

    inline FrameInput ReadFrameInput(const Input::InputSystem& input)
    {
        FrameInput result;

        result.quit = input.Key(GLFW_KEY_ESCAPE).pressed;
        result.toggleWireframe = input.Key(GLFW_KEY_TAB).pressed;
        result.toggleCulling = input.Key(GLFW_KEY_CAPS_LOCK).pressed;
        result.toggleProjection = input.Key(GLFW_KEY_ENTER).pressed;
        result.toggleBackground = input.Key(GLFW_KEY_B).pressed;
        result.toggleMultiplayer = input.Key(GLFW_KEY_M).pressed;
        
        result.toggleCarousel = input.Key(GLFW_KEY_0).pressed;

        if (input.Key(GLFW_KEY_1).pressed)
            result.requestedScene = 0;
        else if (input.Key(GLFW_KEY_2).pressed)
            result.requestedScene = 1;
        else if (input.Key(GLFW_KEY_3).pressed)
            result.requestedScene = 2;

        result.players[0] = Input::ReadPlayerInput(input, player1Bindings);
        result.players[1] = Input::ReadPlayerInput(input, player2Bindings);

        return result;
    }
    
}
