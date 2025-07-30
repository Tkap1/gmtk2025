
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

#if defined(m_vertex)
void main()
{
	vec3 vertex = vertex_pos;

	gl_Position = projection * view * instance_model * vec4(vertex, 1.0);
	v_color = vertex_color * instance_color;
	v_normal = vertex_normal;
	v_uv = vertex_uv;
}
#endif

#if !defined(m_vertex)

uniform sampler2D in_texture;
uniform sampler2D noise;

out vec4 out_color;

void main()
{
	vec2 uv = v_uv * 2.0 - 1.0;
	vec3 color = vec3(0.0);
	float n0 = texture(noise, v_uv + render_time * 0.1).r;
	float d = length(uv) - n0 * 0.1;

	float n1 = texture(noise, v_uv * 0.5 + render_time * 0.1 + 1000.0).r;
	n1 = max(0.0, n1 - 0.5);
	n1 = pow(n1, 0.1);

	color += v_color.rgb * 1.0;
	color -= v_color.rgb * 0.1 * n1;
	float a = smoothstep(0.5, 0.45, d);
	color *= smoothstep(0.2, 0.3, d);

	out_color = vec4(color, v_color.a * a);
	if(out_color.a <= 0.0) { discard; }
}
#endif