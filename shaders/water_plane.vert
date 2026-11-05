#version 430 core

layout (location = 0) in vec3 aPos;

layout (location = 1) in vec2 aTex;

out vec2 TexCoord;
out vec2 InstanceOffset;

uniform int instanceColumns;
uniform int instanceRows;
uniform float instanceSpacing;

void main()
{
	gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);

	TexCoord = aTex;

	int column = gl_InstanceID % instanceColumns;
	int row = gl_InstanceID / instanceColumns;
	InstanceOffset = vec2(
		float(column) - float(instanceColumns - 1) * 0.5,
		float(row) - float(instanceRows - 1) * 0.5
	) * instanceSpacing;
}