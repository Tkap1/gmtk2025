
#if defined(__EMSCRIPTEN__)

#define m_gl_funcs \
X(PFNGLBUFFERDATAPROC, glBufferData) \
X(PFNGLBUFFERSUBDATAPROC, glBufferSubData) \
X(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays) \
X(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray) \
X(PFNGLGENBUFFERSPROC, glGenBuffers) \
X(PFNGLBINDBUFFERPROC, glBindBuffer) \
X(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer) \
X(PFNGLVERTEXATTRIBIPOINTERPROC, glVertexAttribIPointer) \
X(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray) \
X(PFNGLCREATESHADERPROC, glCreateShader) \
X(PFNGLSHADERSOURCEPROC, glShaderSource) \
X(PFNGLCREATEPROGRAMPROC, glCreateProgram) \
X(PFNGLATTACHSHADERPROC, glAttachShader) \
X(PFNGLLINKPROGRAMPROC, glLinkProgram) \
X(PFNGLCOMPILESHADERPROC, glCompileShader) \
X(PFNGLVERTEXATTRIBDIVISORPROC, glVertexAttribDivisor) \
X(PFNGLDRAWARRAYSINSTANCEDPROC, glDrawArraysInstanced) \
X(PFNGLDRAWELEMENTSINSTANCEDPROC, glDrawElementsInstanced) \
X(PFNGLUNIFORM1FVPROC, glUniform1fv) \
X(PFNGLUNIFORM2FVPROC, glUniform2fv) \
X(PFNGLUNIFORM3FVPROC, glUniform3fv) \
X(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation) \
X(PFNGLUSEPROGRAMPROC, glUseProgram) \
X(PFNGLGETSHADERIVPROC, glGetShaderiv) \
X(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog) \
X(PFNGLGENFRAMEBUFFERSPROC, glGenFramebuffers) \
X(PFNGLBINDFRAMEBUFFERPROC, glBindFramebuffer) \
X(PFNGLFRAMEBUFFERTEXTURE2DPROC, glFramebufferTexture2D) \
X(PFNGLCHECKFRAMEBUFFERSTATUSPROC, glCheckFramebufferStatus) \
X(PFNGLDELETEPROGRAMPROC, glDeleteProgram) \
X(PFNGLDELETESHADERPROC, glDeleteShader) \
X(PFNGLUNIFORM1IPROC, glUniform1i) \
X(PFNGLUNIFORM1FPROC, glUniform1f) \
X(PFNGLDETACHSHADERPROC, glDetachShader) \
X(PFNGLGETPROGRAMIVPROC, glGetProgramiv) \
X(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog) \
X(PFNGLDELETEFRAMEBUFFERSPROC, glDeleteFramebuffers) \
X(PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv) \
X(PFNGLBLENDFUNCSEPARATEPROC, glBlendFuncSeparate) \
X(PFNGLGENERATEMIPMAPPROC, glGenerateMipmap) \
X(PFNGLBINDBUFFERBASEPROC, glBindBufferBase) \

#else
#define m_gl_funcs \
X(PFNGLBUFFERDATAPROC, glBufferData) \
X(PFNGLBUFFERSUBDATAPROC, glBufferSubData) \
X(PFNGLGENVERTEXARRAYSPROC, glGenVertexArrays) \
X(PFNGLBINDVERTEXARRAYPROC, glBindVertexArray) \
X(PFNGLGENBUFFERSPROC, glGenBuffers) \
X(PFNGLBINDBUFFERPROC, glBindBuffer) \
X(PFNGLVERTEXATTRIBPOINTERPROC, glVertexAttribPointer) \
X(PFNGLVERTEXATTRIBIPOINTERPROC, glVertexAttribIPointer) \
X(PFNGLENABLEVERTEXATTRIBARRAYPROC, glEnableVertexAttribArray) \
X(PFNGLCREATESHADERPROC, glCreateShader) \
X(PFNGLSHADERSOURCEPROC, glShaderSource) \
X(PFNGLCREATEPROGRAMPROC, glCreateProgram) \
X(PFNGLATTACHSHADERPROC, glAttachShader) \
X(PFNGLLINKPROGRAMPROC, glLinkProgram) \
X(PFNGLCOMPILESHADERPROC, glCompileShader) \
X(PFNGLVERTEXATTRIBDIVISORPROC, glVertexAttribDivisor) \
X(PFNGLDRAWARRAYSINSTANCEDPROC, glDrawArraysInstanced) \
X(PFNGLDRAWELEMENTSINSTANCEDPROC, glDrawElementsInstanced) \
X(PFNGLUNIFORM1FVPROC, glUniform1fv) \
X(PFNGLUNIFORM2FVPROC, glUniform2fv) \
X(PFNGLUNIFORM3FVPROC, glUniform3fv) \
X(PFNGLGETUNIFORMLOCATIONPROC, glGetUniformLocation) \
X(PFNGLUSEPROGRAMPROC, glUseProgram) \
X(PFNGLGETSHADERIVPROC, glGetShaderiv) \
X(PFNGLGETSHADERINFOLOGPROC, glGetShaderInfoLog) \
X(PFNGLGENFRAMEBUFFERSPROC, glGenFramebuffers) \
X(PFNGLBINDFRAMEBUFFERPROC, glBindFramebuffer) \
X(PFNGLFRAMEBUFFERTEXTURE2DPROC, glFramebufferTexture2D) \
X(PFNGLCHECKFRAMEBUFFERSTATUSPROC, glCheckFramebufferStatus) \
X(PFNGLACTIVETEXTUREPROC, glActiveTexture) \
X(PFNGLDELETEPROGRAMPROC, glDeleteProgram) \
X(PFNGLDELETESHADERPROC, glDeleteShader) \
X(PFNGLUNIFORM1IPROC, glUniform1i) \
X(PFNGLUNIFORM1FPROC, glUniform1f) \
X(PFNGLDETACHSHADERPROC, glDetachShader) \
X(PFNGLGETPROGRAMIVPROC, glGetProgramiv) \
X(PFNGLGETPROGRAMINFOLOGPROC, glGetProgramInfoLog) \
X(PFNGLDELETEFRAMEBUFFERSPROC, glDeleteFramebuffers) \
X(PFNGLUNIFORMMATRIX4FVPROC, glUniformMatrix4fv) \
X(PFNGLBLENDFUNCSEPARATEPROC, glBlendFuncSeparate) \
X(PFNGLGENERATEMIPMAPPROC, glGenerateMipmap) \
X(PFNGLBINDBUFFERBASEPROC, glBindBufferBase) \

#endif

#define X(type, name) static type name = NULL;
m_gl_funcs
#undef X

#define invalid_default_case default: { assert(!"Invalid default case"); }
#define invalid_else else { assert(!"Invalid else"); }

#if defined(_WIN32)
#define m_dll_export extern "C" __declspec(dllexport)
#else // _WIN32
#define m_dll_export
#endif

enum e_game_state1
{
	e_game_state1_default,
	e_game_state1_editor,
};

struct s_lerpable
{
	float curr;
	float target;
};

enum e_player_state
{
	e_player_state_default,
};

enum e_tile_type : s8
{
	e_tile_type_none,
	e_tile_type_block,
	e_tile_type_breakable,
	e_tile_type_spike,
	e_tile_type_upgrade_jump,
	e_tile_type_upgrade_speed,
	e_tile_type_upgrade_anti_spike,
	e_tile_type_upgrade_less_run_time,
	e_tile_type_spawn,
	e_tile_type_goal,
	e_tile_type_upgrade_teleport,
	e_tile_type_upgrade_break_tiles,
	e_tile_type_upgrade_super_speed,
	e_tile_type_platform,
	e_tile_type_upgrade_more_loop_time,
	e_tile_type_count,
};

struct s_editor
{
	e_tile_type curr_tile;
	s_v2 cam_pos;
	float zoom;
};

struct s_map
{
	s_v2i spawn_tile_index;
	e_tile_type tile_arr[c_max_tiles][c_max_tiles];
};

struct s_draw_player
{
	s_v2 head_offset;
	s_v2 left_foot_offset;
	s_v2 right_foot_offset;
};

struct s_player
{
	float last_x_dir;
	float last_x_vel;
	s_v2 pos;
	s_v2 prev_pos;
	s_v2 vel;
	int jumps_done;
	b8 jumping;
	float on_ground_timestamp;

	float jump_timestamp;
	float jump_x_dir;
	float start_run_timestamp;

	s_v2 left_foot_offset;
	s_v2 right_foot_offset;
	s_v2 head_offset;
};

enum e_game_state0
{
	e_game_state0_main_menu,
	e_game_state0_leaderboard,
	e_game_state0_win_leaderboard,
	e_game_state0_options,
	e_game_state0_pause,
	e_game_state0_play,
	e_game_state0_input_name,
};

struct s_key_event
{
	b8 went_down;
	int modifiers;
	int key;
};

struct s_key_state
{
	b8 is_down;
	int half_transition_count;
};

template <int n>
struct s_input_str
{
	b8 visual_pos_initialized;
	s_v2 cursor_visual_pos;
	float last_edit_time;
	float last_action_time;
	s_maybe<int> cursor;
	s_str_builder<n> str;
};

struct s_input_name_state
{
	s_input_str<256> name;
	s_str_builder<64> error_str;
};

struct s_ghost
{
	s_list<s_v2, c_max_ghost_positions> pos_arr;
};

struct s_timed_msg
{
	float spawn_timestamp;
	s_v2 pos;
	s_str_builder<64> builder;
};

struct s_soft_game_data
{
	b8 tried_to_submit_score;
	int update_count;
	s_ghost curr_ghost;
	b8 collected_upgrade_this_run;
	s_maybe<int> super_speed_emitter_index;
	float shake_intensity;
	s_v2 player_pos_when_restart_started;
	float start_screen_shake_timestamp;
	float start_restart_timestamp;
	float super_speed_timestamp;
	float want_to_super_speed_timestamp;
	float teleport_timestamp;
	float want_to_teleport_timestamp;
	float run_start_timestamp;
	float used_shield_timestamp;
	float stop_jump_timestamp;
	float want_to_jump_timestamp;
	s_player player;
	s_entity_manager<s_particle_emitter_a, c_max_particle_emitters> emitter_a_arr;
	s_particle_emitter_b emitter_b_arr[c_max_particle_emitters];
	s_list<s_particle, 65536> particle_arr;
	b8 broken_tile_arr[c_max_tiles][c_max_tiles];

	b8 added_particle_emitter_arr[c_max_tiles][c_max_tiles];
	int particle_emitter_index_arr[c_max_tiles][c_max_tiles];

	s_list<s_timed_msg, 8> timed_msg_arr;
};

enum e_upgrade
{
	e_upgrade_break_tiles,
	e_upgrade_one_more_jump,
	e_upgrade_speed,
	e_upgrade_anti_spike,
	e_upgrade_less_run_time,
	e_upgrade_teleport,
	e_upgrade_super_speed,
	e_upgrade_more_loop_time,
	e_upgrade_count,
};

struct s_hard_game_data
{
	s_state state1;
	int update_count;
	int upgrade_arr[e_upgrade_count];
	b8 consumed_tile_arr[c_max_tiles][c_max_tiles];

	// @Note(tkap, 30/06/2025): Circular buffer
	int ghost_count;
	s_ghost ghost_arr[c_max_ghosts];
};

struct s_input
{
	b8 handled;
	b8 left;
	b8 right;
};

struct s_active_sound
{
	Mix_Chunk* chunk;
	s_play_sound_data data;
	float index;
};

struct s_game
{
	b8 freeze_loop;
	b8 reload_shaders;
	b8 any_key_pressed;
	s_v2 last_debug_teleport_pos;
	s_linear_arena update_frame_arena;
	s_linear_arena render_frame_arena;
	s_circular_arena circular_arena;
	u32 ubo;
	s_texture texture_arr[e_texture_count];
	s_mesh mesh_arr[e_mesh_count];
	s_shader shader_arr[e_shader_count];
	float render_time;
	float update_time;
	f64 accumulator;
	f64 time_before;
	u32 curr_fbo;
	Mix_Chunk* sound_arr[e_sound_count];
	int speed_index;
	s_font font;
	s_rng rng;
	float speed;
	s_input_name_state input_name_state;

	s_list<s_active_sound, 128> active_sound_arr;

	s_editor editor;
	s_map map;

	s_input input;

	s_fbo light_fbo;

	b8 music_volume_clean;
	b8 disable_music;
	b8 dim_player_when_out_of_jumps;
	b8 hide_background;
	b8 hide_timer;
	b8 hide_ghosts;
	b8 disable_screen_shake;
	b8 turn_off_all_sounds;

	int update_count_at_win_time;

	s_state state0;

	s_list<s_leaderboard_entry, c_max_leaderboard_entries> leaderboard_arr;
	b8 leaderboard_received;

	s_str_builder<256> leaderboard_session_token;
	s_str_builder<256> leaderboard_public_uid;
	s_str_builder<256> leaderboard_nice_name;
	s_str_builder<256> leaderboard_player_identifier;
	int leaderboard_player_id;

	s_key_state input_arr[c_max_keys];
	s_list<s_key_event, 128> key_events;
	s_list<char, 128> char_events;

	b8 do_soft_reset;
	b8 do_hard_reset;

	s_hard_game_data hard_data;
	s_soft_game_data soft_data;

	int render_instance_count[e_shader_count][e_texture_count][e_mesh_count];
	int render_instance_max_elements[e_shader_count][e_texture_count][e_mesh_count];
	int render_group_index_arr[e_shader_count][e_texture_count][e_mesh_count];
	void* render_instance_arr[e_shader_count][e_texture_count][e_mesh_count];
	s_list<s_render_group, 128> render_group_arr;
};


#include "generated/generated_game.cpp"