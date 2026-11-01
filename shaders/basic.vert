
#version 460 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;

uniform vec3 uPosition; 
uniform vec3 uScale;
uniform float uRotation; 

out vec3 ourColor;

void main()
{
    vec3 drawScale = aPos * uScale;  
    
    float cosAngle = cos(uRotation);
    float sinAngle = sin(uRotation);
    vec2 drawRotation = vec2(cosAngle * drawScale.x - sinAngle * drawScale.y, 
                            sinAngle * drawScale.x + cosAngle * drawScale.y);

    
    vec3 finalPosition = vec3(drawRotation, drawScale.z) + uPosition;
    
    gl_Position = vec4(finalPosition, 1.0);
    ourColor = aColor;
}