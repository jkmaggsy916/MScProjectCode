#version 450
out vec3 FragColour;

in vec2 TexCoords;
in float height;
in float o_type;

void main()
{
    if (o_type <= 1.5 && o_type >=0.5){
        vec3 colour = vec3(1.0 - (height/2.0) - 0.5);
        FragColour = colour;
    }
    else {
        FragColour = vec3(1.0,0.0,0.0);
    }
    //FragColour = vec3(0.8,0.8,0.8);
    
}