
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
	gl_Position = projection * view * instance_model * vec4(vertex, 1);
	v_color = vertex_color * instance_color;
	v_normal = vertex_normal;
	v_uv = vertex_uv;
}
#endif

#if !defined(m_vertex)

uniform sampler2D in_texture;
uniform sampler2D noise;

out vec4 out_color;

float random (vec2 st) {
	return fract(
		sin(dot(st.xy,vec2(12.9898,78.233)))*43758.5453123
	);
}

void main()
{
	vec2 uv = v_uv * 2.0;
	uv.y = 1.0 - uv.y;
	uv.x *= 16.0/9.0;
	vec2 player = player_pos.xy * vec2(1.0, 1.0) * 0.001;
	uv = sin(uv + vec2(-0.5, 0.5) + player - render_time * 0.01) * 0.5 + 0.5;
	float n = texture(noise, (uv + render_time * 0.05)).r;
	vec3 color = vec3(0.0);
	for(int i = 0; i < 50; i += 1) {
		float fi = float(i);
		float r0 = random(vec2(fi * 3.141, fi * 3.141));
		float r1 = random(vec2(fi * 547.341, fi * 577.341));
		float d = distance(vec2(r0, r1), uv);
		float s = smoothstep(0.1, 0.15, d) * smoothstep(0.15, 0.1, d);
		color.g += s * n * 0.5;
	}
	out_color = vec4(color ,1);
}
#endif