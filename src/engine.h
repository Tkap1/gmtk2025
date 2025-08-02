
#include "gen_meta/engine.h.enums"

enum e_mesh
{
	e_mesh_quad,
	e_mesh_cube,
	e_mesh_sphere,
	e_mesh_line,
	e_mesh_count,
};

enum e_shader
{
	e_shader_mesh,
	e_shader_flat,
	e_shader_circle,
	e_shader_button,
	e_shader_text,
	e_shader_background,
	e_shader_line,
	e_shader_light,
	e_shader_portal,
	e_shader_flat_remove_black,
	e_shader_lightning,
	e_shader_tile_background,
	e_shader_count,
};

global constexpr char* c_shader_path_arr[e_shader_count] = {
	"shaders/mesh.shader",
	"shaders/flat.shader",
	"shaders/circle.shader",
	"shaders/button.shader",
	"shaders/text.shader",
	"shaders/background.shader",
	"shaders/line.shader",
	"shaders/light.shader",
	"shaders/portal.shader",
	"shaders/flat_remove_black.shader",
	"shaders/lightning.shader",
	"shaders/tile_background.shader",
};


enum e_texture
{
	e_texture_white,
	e_texture_noise,
	e_texture_font,
	e_texture_light,
	e_texture_atlas,
	e_texture_count
};

global constexpr char* c_texture_path_arr[e_texture_count] = {
	"assets/white.png",
	"assets/noise.png",
	"",
	"",
	"assets/atlas.png",
};


#pragma pack(push, 1)
struct s_instance_data
{
	s_v4 color;
	s_v2 uv_min;
	s_v2 uv_max;
	float mix_weight;
	s_v4 mix_color;
	s_m4 model;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct s_instance_data1
{
	s_v2 start;
	s_v2 end;
	float width;
	s_v4 color;
};
#pragma pack(pop)

struct s_draw_data
{
	b8 flip_x;
	float mix_weight;
	s_v4 mix_color;
};


enum e_depth_mode
{
	e_depth_mode_no_read_no_write,
	e_depth_mode_read_and_write,
	e_depth_mode_read_no_write,
	e_depth_mode_no_read_yes_write,
};

enum e_cull_mode
{
	e_cull_mode_disabled,
	e_cull_mode_back_ccw,
	e_cull_mode_front_ccw,
	e_cull_mode_back_cw,
	e_cull_mode_front_cw,
};

enum e_blend_mode
{
	e_blend_mode_disabled,
	e_blend_mode_additive,
	e_blend_mode_premultiply_alpha,
	e_blend_mode_multiply,
	e_blend_mode_multiply_inv,
	e_blend_mode_normal,
	e_blend_mode_additive_no_alpha,
};

struct s_text_iterator
{
	int index;
	s_len_str text;
	s_list<s_v4, 4> color_stack;
	s_v4 color;
};


struct s_time_format
{
	int hours;
	int minutes;
	int seconds;
	int milliseconds;
};

struct s_fbo
{
	s_v2i size;
	u32 id;
};

struct s_render_flush_data
{
	s_m4 view;
	s_m4 projection;
	s_m4 light_view;
	s_m4 light_projection;
	e_blend_mode blend_mode;
	e_depth_mode depth_mode;
	e_cull_mode cull_mode;
	s_v3 cam_pos;
	s_fbo fbo;
	s_v3 player_pos;
};


#pragma pack(push, 1)
struct s_ply_face
{
	s8 index_count;
	int index_arr[3];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct s_vertex
{
	s_v3 pos;
	s_v3 normal;
	s_v4 color;
	s_v2 uv;
};
#pragma pack(pop)


struct s_ply_mesh
{
	int vertex_count;
	int face_count;
	s_vertex vertex_arr[c_max_vertices];
	s_ply_face face_arr[c_max_faces];
};

struct s_obj_face
{
	int vertex_index[3];
	int normal_index[3];
	int uv_index[3];
};

struct s_obj_vertex
{
	s_v3 pos;
	s_v3 color;
};

struct s_obj_mesh
{
	int vertex_count;
	int normal_count;
	int uv_count;
	int face_count;
	s_obj_vertex vertex_arr[c_max_vertices];
	s_v3 normal_arr[c_max_vertices];
	s_v2 uv_arr[c_max_vertices];
	s_obj_face face_arr[c_max_faces];
};


struct s_render_group
{
	int element_size;
	e_shader shader_id;
	e_texture texture_id;
	e_mesh mesh_id;
};


struct s_glyph
{
	int advance_width;
	int width;
	int height;
	int x0;
	int y0;
	int x1;
	int y1;
	s_v2 uv_min;
	s_v2 uv_max;
};

struct s_texture
{
	u32 id;
};

struct s_font
{
	float size;
	float scale;
	int ascent;
	int descent;
	int line_gap;
	s_glyph glyph_arr[1024];
};

struct s_vbo
{
	u32 id;
	int max_elements;
};

struct s_mesh
{
	int vertex_count;
	u32 vao;
	u32 vertex_vbo;
	s_vbo instance_vbo;
};

struct s_shader
{
	u32 id;
};

struct s_timer
{
	float want_to_use_timestamp;
	float used_timestamp;
};

enum e_emitter_spawn_type
{
	e_emitter_spawn_type_point,
	e_emitter_spawn_type_circle,
	e_emitter_spawn_type_sphere,
	e_emitter_spawn_type_rect_center,
	e_emitter_spawn_type_rect_edge,
	e_emitter_spawn_type_circle_outline,
};

struct s_particle_color
{
	b8 color_rand_per_channel;
	s_v4 color;
	float color_rand;
	float percent;
};

struct s_particle_emitter_a
{
	b8 follow_emitter;
	s_v3 pos;
	float shrink;
	float particle_duration;
	float particle_duration_rand;
	float radius;
	float radius_rand;
	s_v3 dir;
	s_v3 dir_rand;
	float gravity;
	float speed;
	float speed_rand;
	s_list<s_particle_color, 4> color_arr;
};

struct s_particle_emitter_b
{
	b8 paused;
	b8 pause_after_update;
	b8 existed_in_previous_frame;
	float duration;
	float creation_timestamp;
	float last_emit_timestamp;
	float particles_per_second;
	int particle_count;
	int num_alive_particles;
	e_emitter_spawn_type spawn_type;
	s_v3 spawn_data;
};

struct s_particle
{
	int emitter_index;
	u64 seed;
	s_v3 pos;
	float spawn_timestamp;
};

data_enum(e_entity,

	s_entity_type_data
	g_entity_type_data

	player {
		.max_count = 1,
	}
	enemy {
		.max_count = 1024,
	}
	dying_enemy {
		.max_count = 1024,
	}
	emitter {
		.max_count = 1024,
	}
	fct {
		.max_count = 256,
	}
	visual_effect {
		.max_count = 256,
	}
)

struct s_entity_type_data
{
	int max_count;
};

template <typename t, int n>
struct s_entity_manager
{
	int count[e_entity_count];
	b8 active[n];
	int* free_list[e_entity_count];
	t data[n];
};


#include "generated/generated_engine.cpp"
#include "gen_meta/engine.h.globals"


func constexpr int get_max_entities()
{
	int result = 0;
	for_enum(type_i, e_entity) {
		result += g_entity_type_data[type_i].max_count;
	}
	return result;
}

func constexpr int get_max_entities_of_type(e_entity type)
{
	int result = g_entity_type_data[type].max_count;
	return result;
}

func constexpr int get_first_index(e_entity type)
{
	int result = 0;
	for_enum(type_i, e_entity) {
		if(type_i == type) { break; }
		result += g_entity_type_data[type_i].max_count;
	}
	return result;
}

func constexpr int get_last_index_plus_one(e_entity type)
{
	int result = 0;
	for_enum(type_i, e_entity) {
		result += g_entity_type_data[type_i].max_count;
		if(type_i == type) { break; }
	}
	return result;
}


global constexpr int c_max_entities = get_max_entities();
global constexpr int c_first_index[e_entity_count] = {
	get_first_index(e_entity_player),
	get_first_index(e_entity_enemy),
	get_first_index(e_entity_dying_enemy),
	get_first_index(e_entity_emitter),
	get_first_index(e_entity_fct),
	get_first_index(e_entity_visual_effect),
};

global constexpr int c_last_index_plus_one[e_entity_count] = {
	get_last_index_plus_one(e_entity_player),
	get_last_index_plus_one(e_entity_enemy),
	get_last_index_plus_one(e_entity_dying_enemy),
	get_last_index_plus_one(e_entity_emitter),
	get_last_index_plus_one(e_entity_fct),
	get_last_index_plus_one(e_entity_visual_effect),
};
