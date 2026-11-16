
#version 460 core

in vec3 Normal;
in vec2 TextCoord; 
in vec3 VertexColor;

out vec4 FragColor;


uniform bool uColorOverride = false;
uniform float uColorAmount = 1.0f;           // 0 = vertex color, 1 = solid color.
uniform vec3 uObjectColor = vec3(1.0);       

uniform bool uUseTexture = false;
uniform float uTintTextureColorAmount = 0.0f;    // 0 = texture RGB, 1 = chosen color.
uniform sampler2D uTexture2D;

uniform bool uUseLighting = false; 
uniform vec3 ulightColor = vec3(1.0);
uniform vec3 ulightDirection= vec3(-1.0f, -0.6f, -0.3f);


void main()
{
    // Keep vertex colors unless the override is enabled.
    float colorAmount = uColorOverride ? uColorAmount : 0.0f;
    // Blend vertex colors toward the supplied solid color.
    vec3 chosenColor = mix(
        VertexColor, 
        uObjectColor, 
        colorAmount
    );
    
    // the untextured, not affected by light color 
    vec4 baseColor = vec4(chosenColor, 1.0);
    
    
    if (uUseTexture)
    {
        // Sample the texture at the interpolated UV coordinates.
        vec4 textureColor = texture(uTexture2D, TextCoord);

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
        baseColor = vec4(result, textureColor.a);
    }
    
    
    if (uUseLighting)
    {
        vec3 normal = normalize(Normal);
        vec3 directionToLight = normalize(-ulightDirection);

        float diffuse = max(dot(normal, directionToLight), 0.0);
        float brightness = 0.2 + 0.8 * diffuse;

        baseColor.rgb *= ulightColor * brightness;
    }
    
    FragColor = baseColor;
}


// Note: less if-cases 