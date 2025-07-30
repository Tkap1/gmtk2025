
#if defined(m_vertex)
layout (location = 0) in vec2 instance_start;
layout (location = 1) in vec2 instance_end;
layout (location = 2) in float instance_width;
layout (location = 3) in vec4 instance_color;
#endif

shared_var vec4 v_color;

#if defined(m_vertex)
void main()
{
	float hw = instance_width * 0.5;
	vec2 dir = normalize(instance_end - instance_start);
	vec2 perp = vec2(dir.y, dir.x);
	vec2 pos[6] = vec2[](
		vec2(instance_start.x + perp.x * hw, instance_start.y - perp.y * hw),
		vec2(instance_end.x + perp.x * hw, instance_end.y - perp.y * hw),
		vec2(instance_end.x - perp.x * hw, instance_end.y + perp.y * hw),
		vec2(instance_start.x + perp.x * hw, instance_start.y - perp.y * hw),
		vec2(instance_end.x - perp.x * hw, instance_end.y + perp.y * hw),
		vec2(instance_start.x - perp.x * hw, instance_start.y + perp.y * hw)
	);
	vec2 vertex = pos[gl_VertexID];
	gl_Position = projection * view * vec4(vertex, 0.0, 1.0);
	v_color = instance_color;
}
#endif

#if !defined(m_vertex)

uniform sampler2D in_texture;

out vec4 out_color;

void main()
{
	out_color = v_color;
}
#endif