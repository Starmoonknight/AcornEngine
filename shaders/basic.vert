
#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
layout(location = 2) in vec3 aNormal;
layout(location = 3) in vec2 aTexCoord;

out vec3 vertexColor;


uniform mat4 uModel;
uniform mat4 uView; 
uniform mat4 uProjection; 

void main()
{
    vec4 localPosition = vec4(aPos, 1.0);
    
    /* Applying to a vertex:
    *
    *      translationMatrix * (
    *          rotationMatrix * (
    *              scaleMatrix * originalPosition
    *          )
    *      )
    */
    vec4 worldPosition = uModel * localPosition;
    
    vec4 viewPosition = uView * worldPosition;
    
    gl_Position = uProjection * viewPosition;
    
    vertexColor = aColor;
}


