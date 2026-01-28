#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;
in vec4 vertexTangent;

// Input uniform values
uniform mat4 mvp;
uniform mat4 matModel;
uniform mat4 matNormal;

// Output vertex attributes (to fragment shader)
out vec3 fragPosition;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragNormal;
out vec3 fragTangent;
out vec3 fragBitangent;

void main()
{
    // Send vertex attributes to fragment shader
    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    
    // Calculate TBN matrix for normal mapping
    vec3 T = normalize(vec3(matNormal * vec4(vertexTangent.xyz, 0.0)));
    vec3 N = normalize(vec3(matNormal * vec4(vertexNormal, 0.0)));
    
    // Re-orthogonalize T with respect to N
    T = normalize(T - dot(T, N) * N);
    
    // Calculate bitangent using the tangent w component (handedness)
    vec3 B = cross(N, T) * vertexTangent.w;
    
    fragNormal = N;
    fragTangent = T;
    fragBitangent = B;
    
    // Calculate final vertex position
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
