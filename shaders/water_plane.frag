#version 430 core

in vec2 TexCoord;

out vec4 FragColor;

vec3 rgb(int r, int g, int b)
{
        return vec3(r, g, b)/255.0;
}

void main()
{
        float factor = sin(TexCoord.x * 100.0) / 2 + 0.5;
        factor = clamp((factor * factor) - 0.5, 0.0, 1.0);

        FragColor = vec4(vec3(mix(rgb(41, 67, 95), rgb(42, 106, 150), factor)), 1.0f);
} 