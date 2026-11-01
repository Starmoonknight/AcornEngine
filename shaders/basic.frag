
#version 460 core

in vec3 ourColor;
out vec4 FragColor;

uniform bool uColorOverride = false;
uniform vec3 uSolidColor;

void main()
{
    if (uColorOverride == true)
    {
        FragColor = vec4(uSolidColor, 1.0);
    }
    else
    {        
        FragColor = vec4(ourColor, 1.0);
    }
}