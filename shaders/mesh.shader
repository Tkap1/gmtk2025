
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
shared_var vec4 v_light_frag_pos;
shared_var vec3 v_frag_pos;
shared_var vec2 v_uv;

#if defined(m_vertex)
void main()
{
	vec3 vertex = vertex_pos;
	gl_Position = projection * view * instance_model * vec4(vertex, 1);
	v_color = vertex_color * instance_color;
	v_light_frag_pos = light_projection * light_view * instance_model * vec4(vertex, 1);
	// v_uv = vertex_uv;
	v_frag_pos = (instance_model * vec4(vertex_pos, 1)).xyz;

	vec2 uv;
	if(abs(vertex_normal.x) > 0.9) {
		uv = vec2(vertex_pos.y, vertex_pos.z) + 0.5;
		if(vertex_normal.x < 0.0) { uv.x = 1.0 - uv.x; }
		uv.y = 1.0 - uv.y;
	}
	else if(abs(vertex_normal.y) > 0.9) {
		uv = vec2(vertex_pos.x, vertex_pos.z) + 0.5;
		uv.y = 1.0 - uv.y;
		if(vertex_normal.y > 0.0) { uv.x = 1.0 - uv.x; }
	}
	else {
		uv = vec2(vertex_pos.x, vertex_pos.y) + 0.5;
		if(vertex_normal.z > 0.0) { uv.y = 1.0 - uv.y; }
	}

	v_uv = uv;

	mat3 normal_matrix = transpose(inverse(mat3(instance_model)));
	v_normal = normalize(normal_matrix * vertex_normal);
}
#endif

#if !defined(m_vertex)

uniform sampler2D in_texture;

out vec4 out_color;

void main()
{
	vec3 normal = normalize(v_normal);
	// vec3 color = normal * 0.5 + 0.5;
	vec3 color = vec3(0.0);

	vec3 light_dir = normalize(vec3(1.0, 1.0, -1.0));

	vec4 texture_color = texture(in_texture, v_uv);

	float d = max(0.0, dot(-light_dir, normal)) * 0.5 + 0.5;
	color = v_color.rgb * texture_color.rgb * d;

	// color = vec3(d);
	// color = vec3(curr_depth);
	// color = vec3(shadow);
	// if(temp.x < 0.0 || temp.x > 1.0) {
	// 	color = vec3(0.0, 1.0, 0.0);
	// }
	// if(temp.y < 0.0 || temp.y > 1.0) {
	// 	color = vec3(1.0, 0.0, 0.0);
	// }
	// if(curr_depth < 0.0) {
	// 	color = vec3(0.0, 0.0, 1.0);
	// }
	// else if(curr_depth <= 0.0) {
	// 	color = vec3(1.0, 0.0, 1.0);
	// }
	out_color = vec4(color, 1.0);
}
#endif