#shader vertex
#version 330 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 velocity;
layout(location = 2) in vec2 acceleration;
layout(location = 3) in vec3 colour;

out vec4 v_Colour;

void main()
{
	gl_Position = vec4(position.x, position.y, 0.0, 1.0);
	gl_PointSize = 2.0;
	v_Colour = vec4(colour.x, colour.y, colour.z, 1.0);
};


#shader fragment
#version 330 core

in vec4 v_Colour;
layout(location = 0) out vec4 f_Colour;

void main()
{
	if (length(gl_PointCoord - vec2(0.5)) > 0.5)
		discard;

	f_Colour = v_Colour;
};