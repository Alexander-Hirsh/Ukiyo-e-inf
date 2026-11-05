// tessellation control shader
#version 430 core

// specify number of control points per patch output
// this value controls the size of the input and output arrays
layout (vertices=4) out;

// varying input from vertex shader
in vec2 TexCoord[];
in vec2 InstanceOffset[];
// varying output to evaluation shader
out vec2 TextureCoord[];
out vec2 PatchInstanceOffset[];

uniform int tessLevel;

void main()
{
        // ----------------------------------------------------------------------
        // pass attributes through
        gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
        TextureCoord[gl_InvocationID] = TexCoord[gl_InvocationID];
        PatchInstanceOffset[gl_InvocationID] = InstanceOffset[gl_InvocationID];

        // ----------------------------------------------------------------------
        // invocation zero controls tessellation levels for the entire patch
        if (gl_InvocationID == 0)
        {
                gl_TessLevelOuter[0] = tessLevel;
                gl_TessLevelOuter[1] = tessLevel;
                gl_TessLevelOuter[2] = tessLevel;
                gl_TessLevelOuter[3] = tessLevel;

                gl_TessLevelInner[0] = tessLevel;
                gl_TessLevelInner[1] = tessLevel;
        }
}