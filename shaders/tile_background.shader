
#if defined(m_vertex)
layout (location = 0) in vec3 vertex_pos;
layout (location = 1) in vec3 vertex_normal;
layout (location = 2) in vec4 vertex_color;
layout (location = 3) in vec2 vertex_uv;
layout (location = 4) in vec4 instance_color;
layout (location = 5) in vec4 instance_uv_min;
layout (location = 6) in vec4 instance_uv_max;
layout (location = 7) in float instance_mix_weight;
layout (location = 8) in vec4 instance_mix_color;
layout (location = 9) in mat4 instance_model;
#endif

shared_var vec4 v_color;
shared_var vec3 v_normal;
shared_var vec2 v_vertex_uv;
shared_var vec2 v_instance_uv;
shared_var float v_mix_weight;
shared_var vec4 v_mix_color;
shared_var vec2 v_size;

#if defined(m_vertex)
void main()
{
	vec3 vertex = vertex_pos;
	gl_Position = projection * view * instance_model * vec4(vertex, 1);
	v_color = vertex_color * instance_color;
	v_normal = vertex_normal;
	v_vertex_uv = vertex_uv;
	v_mix_weight = instance_mix_weight;
	v_mix_color = instance_mix_color;
	v_size = vec2(instance_model[0][0], instance_model[1][1]);

	vec2 uv_arr[6] = vec2[](
		vec2(instance_uv_min.x, instance_uv_min.y),
		vec2(instance_uv_max.x, instance_uv_min.y),
		vec2(instance_uv_max.x, instance_uv_max.y),
		vec2(instance_uv_min.x, instance_uv_min.y),
		vec2(instance_uv_max.x, instance_uv_max.y),
		vec2(instance_uv_min.x, instance_uv_max.y)
	);
	v_instance_uv = uv_arr[gl_VertexID % 6];
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
	vec2 uv = v_vertex_uv;
	uv.x *= v_size.x / v_size.y;
	vec2 id = floor(uv * 16);
	float r = random(id);
	float foo = pow(r + 0.4, 0.25) * 0.4;
	float n = texture(noise, uv * 4.0 / foo).r;
	out_color.g = foo * pow(n, 0.5) * 0.75;
	out_color.a = 1.0;
}
#endif