#version 330

in vec3 vertexPosition;

uniform mat4 mvp;

out vec3 fragTexCoord;

void main()
{
    fragTexCoord = vertexPosition;
    vec4 pos = mvp * vec4(vertexPosition, 1.0);
    gl_Position = pos.xyww;  // Ensure skybox is always at far plane
}
