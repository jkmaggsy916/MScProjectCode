#version 450 core
in vec3 vertex;
in vec2 texCoords;
in float type;

uniform mat4 projMatrix;
uniform mat4 ballMatrix;

void main()
{
    if(type <= 1.5 && type >=0.5){
        gl_Position = projMatrix * vec4(vertex.x, 0.0 ,vertex.y, 1.0);
    }
    else if(type <= 2.5 && type > 1.5){
        gl_Position = projMatrix *  ballMatrix * vec4(vertex, 1.0);
    }
} 