
#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;
layout(location = 3) in vec3 aColor;

out vec3 Normal;
out vec2 TextCoord; 
out vec3 VertexColor;

uniform mat4 uModel;    
uniform mat4 uView;     
uniform mat4 uProjection;

uniform bool uUseLighting = false;


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
    // Transform from local space to world space.
    vec4 worldPosition = uModel * localPosition;
    
    // Transform from world space to camera space.
    vec4 viewPosition = uView * worldPosition;
    
    // Project into clip space.
    gl_Position = uProjection * viewPosition;
    
    VertexColor = aColor;
    TextCoord = aTexCoord;
    
    
    // some of my basic shapes don't really suport normals so doing it like this for now, will probably remove them later 
    Normal = vec3(0.0);
    
    if (uUseLighting)
    {
        // transform normals from local-model space into world space 
        mat3 normalMatrix = transpose(inverse(mat3(uModel)));
        Normal = normalMatrix * aNormal;
    }
}

// Note: move normal calculatio to cpu? 
// Note: look into calculating normals if missing? At face appending step, save normals for it.
