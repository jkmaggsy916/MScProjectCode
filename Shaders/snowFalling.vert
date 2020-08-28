#version 450
in vec3 vertex;

out vec3 vert;

uniform mat4 projMatrix;
uniform mat4 mvMatrix;
uniform mat3 normalMatrix;

void main() {
   vert = vertex.xyz;
   gl_Position = vec4(vertex,1.0);
}