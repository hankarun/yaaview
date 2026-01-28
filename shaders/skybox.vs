#version 330

in vec3 vertexPosition;

uniform mat4 mvp;

out vec3 fragTexCoord;

void main()
{
    fragTexCoord = vertexPosition;
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
