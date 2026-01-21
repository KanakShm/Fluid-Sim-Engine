#shader vertex
#version 330 core

layout(location = 0) in vec3 pose;
layout(location = 1) in vec2 texCoord;

out vec2 v_TexCoord;

uniform mat4 u_MVP;
uniform float offset;

void main()
{
	gl_Position = u_MVP * vec4(pose.x + offset, pose.y, pose.z, 1.0);
	v_TexCoord = texCoord;
};


#shader fragment
#version 330 core

out vec4 fragColour;

in vec2 v_TexCoord;

uniform float blend;
uniform sampler2D u_Texture1;
uniform sampler2D u_Texture2;

void main()
{
	fragColour = mix(texture(u_Texture1, v_TexCoord), texture(u_Texture2, v_TexCoord), blend);
};