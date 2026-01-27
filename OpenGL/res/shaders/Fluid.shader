#shader vertex
#version 330 core

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 velocity;
layout(location = 2) in vec2 acceleration;
layout(location = 3) in float density;
layout(location = 4) in float pressure;
layout(location = 5) in vec2 F_pressure;
layout(location = 6) in vec2 F_viscocity;
layout(location = 7) in vec2 F_other;
layout(location = 8) in vec3 colour;

out vec4 v_Colour;

float max_visual_speed = 4.0f;

void main()
{
	gl_Position = vec4(position.x, position.y, 0.0, 1.0);
	gl_PointSize = 5.0;

	float speed = length(velocity);
	float t = clamp(speed / max_visual_speed, 0.0f, 1.0f);
	t =	clamp(t, 0.0f, 1.0f);

	vec4 blue   = vec4(0.0, 0.5, 1.0, 1.0);
    vec4 yellow = vec4(1.0, 1.0, 0.0, 1.0);
    vec4 orange = vec4(1.0, 0.5, 0.0, 1.0);
    vec4 red    = vec4(1.0, 0.0, 0.0, 1.0);

    if (t < 0.5) {
        float local_t = t / 0.5;
        v_Colour = mix(blue, yellow, local_t);
    } 
    else if (t < 0.66) {
        float local_t = (t - 0.33) / 0.33;
        v_Colour = mix(yellow, orange, local_t);
    } 
    else {
        float local_t = (t - 0.66) / 0.34;
        v_Colour = mix(orange, red, local_t);
    }
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