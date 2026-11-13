
#version 460 core

in vec3 vertexColor;
in vec2 textCoord; 

out vec4 FragColor;


uniform bool uColorOverride = false;
uniform vec3 uSolidColor = vec3(1.0);       
uniform float uColorAmount = 1.0;           // 0 = vertex color, 1 = solid color.

uniform bool uUseTexture = false;
uniform sampler2D uTexture2D;
uniform float uTextureColorAmount = 0.0;    // 0 = texture RGB, 1 = chosen color.


void main()
{
    // Keep vertex colors unless the override is enabled.
    float colorAmount = uColorOverride ? uColorAmount : 0.0f;
    // Blend vertex colors toward the supplied solid color.
    vec3 chosenColor = mix(
        vertexColor, 
        uSolidColor, 
        colorAmount
    );
    
    if (uUseTexture)
    {
        // Sample the texture at the interpolated UV coordinates.
        vec4 textureColor = texture(uTexture2D, textCoord);

        // Blend texture RGB toward the chosen color; preserve texture alpha.
        FragColor = vec4(
                mix(textureColor.rgb, chosenColor, uTextureColorAmount), 
                textureColor.a
        ); 
    }
    else
    {
        // Use the chosen color with full opacity.
        FragColor = vec4(chosenColor, 1.0);
    }
}