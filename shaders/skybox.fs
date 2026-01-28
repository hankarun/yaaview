#version 330

in vec3 fragTexCoord;

uniform samplerCube environmentMap;
uniform float exposure;

out vec4 finalColor;

void main()
{
    vec3 color = texture(environmentMap, fragTexCoord).rgb;
    
    // HDR tone mapping
    color = vec3(1.0) - exp(-color * exposure);
    
    // Gamma correction
    color = pow(color, vec3(1.0/2.2));
    
    finalColor = vec4(color, 1.0);
}
