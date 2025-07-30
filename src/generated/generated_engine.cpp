func s_render_flush_data make_render_flush_data(s_v3 cam_pos, s_v3 player_pos);
func s_mesh make_mesh_from_vertices(s_vertex* vertex_arr, int vertex_count);
func s_ply_mesh parse_ply_mesh(char* path, s_linear_arena* arena);
func s_mesh make_mesh_from_ply_file(char* file, s_linear_arena* arena);
func s_mesh make_mesh_from_obj_file(char* file, s_linear_arena* arena);
func void set_cull_mode(e_cull_mode mode);
func void set_depth_mode(e_depth_mode mode);
func void set_blend_mode(e_blend_mode mode);
func void set_window_size(int width, int height);
func Mix_Chunk* load_sound_from_file(char* path, u8 volume);
func void play_sound(e_sound sound_id, s_play_sound_data data);
func s_texture load_texture_from_file(char* path, u32 filtering);
func s_texture load_texture_from_data(void* data, int width, int height, int format, u32 filtering);
func s_font load_font_from_file(char* file, int font_size, s_linear_arena* arena);
func s_v2 draw_text(s_len_str text, s_v2 in_pos, float font_size, s_v4 color, b8 centered, s_font* font);
func s_v2 get_text_size_with_count(s_len_str in_text, s_font* font, float font_size, int count, int in_column);
func s_v2 get_text_size(s_len_str text, s_font* font, float font_size);
func b8 iterate_text(s_text_iterator* it, s_len_str text, s_v4 color);
func int hex_str_to_int(s_len_str str);
func s_len_str format_text(const char* text, ...);
func u8* try_really_hard_to_read_file(char* file, s_linear_arena* arena);
func float update_time_to_render_time(float time, float interp_dt);
func s_m4 fullscreen_m4();
func s_time_format update_time_to_time_format(float update_time);
func s_time_format update_count_to_time_format(int update_count);
func s_obj_mesh* parse_obj_mesh(char* path, s_linear_arena* arena);
func char* skip_whitespace(char* str);
func s_v2 wxy(float x, float y);
func s_v2 wcxy(float x, float y);
func void update_particles(float delta);
func int add_emitter(s_particle_emitter_a a, s_particle_emitter_b b);
template <typename t, int n>
func int entity_manager_add(s_entity_manager<t, n>* manager, t new_entity);
template <typename t, int n>
func void entity_manager_remove(s_entity_manager<t, n>* manager, int index);
template <typename t, int n>
func void entity_manager_reset(s_entity_manager<t, n>* manager);
func s_v4 get_particle_color(s_rng* rng, float percent, s_list<s_particle_color, 4> color_arr);
