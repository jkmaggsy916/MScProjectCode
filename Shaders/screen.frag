#version 450 core

out vec4 FragColor;

in vec2 TexCoords;

uniform sampler2D snowFallingTexture;
uniform sampler2D snowGroundTexture;
uniform sampler2D debugTexture;
uniform bool debugEnabled;


void main()
{
    bool debug = debugEnabled;

    vec3 col;
    vec3 test = texture(snowGroundTexture, TexCoords).rgb;
    if(test.x <= 0.25 && test.y <= 0.25 && test.z <= 0.25){
        col = texture(snowGroundTexture, TexCoords).rgb + texture(snowFallingTexture, TexCoords).rgb;
    }
    else{
        col = mix(texture(snowGroundTexture, TexCoords).rgb, texture(snowFallingTexture, TexCoords).rgb, 0.25);
    }
    if (debug == true){
        col = texture(debugTexture, TexCoords).rgb;
    }
    //vec3 col = texture(snowGroundTexture, TexCoords).rgb + texture(snowFallingTexture, TexCoords).rgb;
    //vec3 col = texture(snowFallingTexture, TexCoords).rgb;
    FragColor = vec4(col, 1.0);
} 