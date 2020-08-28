#version 450
in vec3 vertex;
in vec2 texCoords;
in float type;

uniform mat4 projMatrix;
uniform mat4 mvMatrix;
uniform mat4 ballMatrix;

uniform sampler2D noiseTexture;
uniform sampler2D heightTexture;

out vec2 TexCoords;
out float height;
out float o_type;

uint pack_rgba(vec4 heightvec){
    vec4 int_vec = vec4(int(255 * heightvec.r), int(255 * heightvec.g), int(255 * heightvec.b), int(255 * heightvec.a));
    return uint(int(int_vec.r)<<24|int(int_vec.g)<<16|int(int_vec.b)<<8|int(int_vec.a));
}

vec4 unpack_rgba(uint a) {
    return vec4(a>>24 & 255 , a >>16 & 255, a >>8 & 255, a >>0 & 255) / 255.0;
}

void main()
{
    if(type <= 1.5 && type >=0.5){
        TexCoords = texCoords;
        vec4 test = texture(noiseTexture, texCoords);
        uint h = pack_rgba(test);
        float n = float(h) / 4294967294.0; 
        height = -1.0*n;
        float heightmapHeight = texture(heightTexture,vec2(texCoords.y,-texCoords.x)).r - 0.6;
        if(heightmapHeight >=0.35){
            heightmapHeight = 0.0;
        }
        //n += heightmapHeight;
        gl_Position = projMatrix * mvMatrix * vec4(vertex.x, -5.0*heightmapHeight + n/20.0,vertex.y, 1.0);
        //gl_Position = projMatrix * mvMatrix * vec4(vertex.x, 0.0 ,vertex.y, 1.0);

        o_type = type;
    }
    else if(type <= 2.5 && type > 1.5){
        height = 1.0;
        o_type = type;
        gl_Position = projMatrix * mvMatrix * ballMatrix * vec4(vertex.x ,vertex.y, vertex.z, 1.0);
    }
    
}