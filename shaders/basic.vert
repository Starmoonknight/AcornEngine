
#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform vec3 uPosition = vec3(0.0f); 
uniform vec3 uScale = vec3(1.0f);
uniform float uRotation = 0; 

out vec3 outColor;

void main()
{
    vec3 scaledPos = aPos * uScale;  
    
    float c = cos(uRotation);
    float s = sin(uRotation);
    
    vec2 rotatedPos = vec2(
            c * scaledPos.x - s * scaledPos.y, 
            s * scaledPos.x + c * scaledPos.y);
    
    
    vec3 finalPosition = vec3(rotatedPos, scaledPos.z) + uPosition;
    
    gl_Position = vec4(finalPosition, 1.0);
    outColor = aColor;
}