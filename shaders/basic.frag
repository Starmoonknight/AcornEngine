
#version 460 core

in vec3 vertexColor;
in vec2 textCoord; 

out vec4 FragColor;


uniform bool uColorOverride = false;
uniform vec3 uSolidColor = vec3(1.0);       
uniform float uColorAmount = 1.0f;           // 0 = vertex color, 1 = solid color.

uniform bool uUseTexture = false;
uniform float uTintTextureColorAmount = 0.0f;    // 0 = texture RGB, 1 = chosen color.
uniform sampler2D uTexture2D;


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

        // Black darkens, white brightens, and 0.5 leaves the image unchanged.
        // Converts the chosen colour's 0–1 range into a direction from -1 to +1:   -1 = darken, and +1 = brighten
        vec3 tintDirection = 2.0 * chosenColor - vec3(1.0);

        // Reduce changes near fully black and fully white image pixels.
        vec3 change = tintDirection 
                    * textureColor.rgb 
                    * (vec3(1.0) - textureColor.rgb);

        vec3 result = textureColor.rgb 
                    + uTintTextureColorAmount * change;
        
        // This should allow using black and white as a tint color
        FragColor = vec4(result, textureColor.a);
    }
    else
    {
        // Use the chosen color with full opacity.
        FragColor = vec4(chosenColor, 1.0);
    }
}