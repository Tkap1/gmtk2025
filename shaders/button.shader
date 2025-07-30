
#if defined(m_vertex)
layout (location = 0) in vec3 vertex_pos;
layout (location = 1) in vec3 vertex_normal;
layout (location = 2) in vec4 vertex_color;
layout (location = 3) in vec2 vertex_uv;
layout (location = 4) in vec4 instance_color;
layout (location = 5) in vec4 instance_uv_min;
layout (location = 6) in vec4 instance_uv_max;
layout (location = 7) in mat4 instance_model;
#endif

shared_var vec4 v_color;
shared_var vec3 v_normal;
shared_var vec2 v_uv;
shared_var vec2 v_pos;
shared_var vec2 v_size;

#if defined(m_vertex)
void main()
{
	vec3 vertex = vertex_pos;
	gl_Position = projection * view * instance_model * vec4(vertex, 1);
	v_color = vertex_color * instance_color;
	v_normal = vertex_normal;
	v_uv = vertex_uv;
	v_pos = vec2(instance_model[3][0], instance_model[3][1]);
	v_size = vec2(instance_model[0][0], instance_model[1][1]);
}
#endif

#if !defined(m_vertex)

uniform sampler2D in_texture;

out vec4 out_color;

float sdRoundedRect(vec2 p, vec2 b, float r) {
	vec2 q = abs(p) - b + vec2(r);
	return length(max(q, 0.0)) - r + min(max(q.x, q.y), 0.0);
}

void main()
{
	vec2 center = v_pos + v_size * 0.5;
	vec2 p = (v_pos + v_size * v_uv) - center;
	float d = sdRoundedRect(p, v_size * 0.5, 20.0);
	float edge = fwidth(d);
	float alpha = smoothstep(0.0, edge, -d);

	vec4 texture_color = texture(in_texture, v_uv);
	vec4 color = texture_color * v_color;

	vec2 uv = v_uv;
	uv.y = 1.0 - uv.y;
	float mouse_dist = distance(v_pos - v_size * 0.5 + v_size * uv, mouse);
	color.rgb += smoothstep(150.0, 0.0, mouse_dist) * 0.2 * vec3(1.0, 0.5, 0.5);

	color.a *= alpha;
	out_color = color;
}
#endif