

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


struct s_sound_data
{
	char* path;
	u8 volume;
};

enum e_sound
{
	e_sound_click,
	e_sound_key,
	e_sound_break,
	e_sound_jump1,
	e_sound_jump2,
	e_sound_clap,
	e_sound_land,
	e_sound_restart,
	e_sound_super_speed,
	e_sound_shield,
	e_sound_teleport,
	e_sound_count,
};

global constexpr s_sound_data c_sound_data_arr[e_sound_count] = {
	{"assets/click.wav", 128},
	{"assets/key.wav", 128},
	{"assets/break.wav", 128},
	{"assets/jump1.wav", 128},
	{"assets/jump2.wav", 128},
	{"assets/clap.wav", 255},
	{"assets/land.wav", 255},
	{"assets/restart.wav", 128},
	{"assets/super_speed.wav", 255},
	{"assets/shield.wav", 255},
	{"assets/teleport.wav", 255},
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

enum e_emitter_spawn_type
{
	e_emitter_spawn_type_point,
	e_emitter_spawn_type_circle,
	e_emitter_spawn_type_sphere,
	e_emitter_spawn_type_rect_center,
	e_emitter_spawn_type_rect_edge,
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
	float particles_per_second = 1;
	int particle_count = 1;
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

template <typename t, int n>
struct s_entity_manager
{
	int count;
	b8 active[n];
	int free_list[n];
	t data[n];
};



#include "generated/generated_engine.cpp"