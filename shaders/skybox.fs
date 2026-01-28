#version 330

in vec3 fragTexCoord;

uniform samplerCube environmentMap;
uniform float exposure;

out vec4 finalColor;

void main()
{
    vec3 color = texture(environmentMap, fragTexCoord).rgb;
    
    // JPG textures are already in sRGB space, so convert to linear first
    color = pow(color, vec3(2.2));
    
    // HDR tone mapping
    color = vec3(1.0) - exp(-color * exposure);
    
    // Gamma correction (back to sRGB)
    color = pow(color, vec3(1.0/2.2));
    
    finalColor = vec4(color, 1.0);
}
