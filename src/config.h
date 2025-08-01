

global constexpr s_v2 c_world_size = {1366, 768};
global constexpr s_v2 c_world_center = {c_world_size.x * 0.5f, c_world_size.y * 0.5f};
global constexpr int c_max_vertices = 16384;
global constexpr int c_max_faces = 8192;
global constexpr s_v3 c_up_axis = {0, 0, 1};
global constexpr int c_updates_per_second = 100;
global constexpr f64 c_update_delay = 1.0 / c_updates_per_second;
global constexpr int c_max_particle_emitters = 1024;
global constexpr float c_transition_time = 0.25f;
global constexpr int c_tile_size = 32;
global constexpr s_v2 c_tile_size_v = {c_tile_size, c_tile_size};
global constexpr int c_max_tiles = 1024;
global constexpr float c_gravity = 0.2f;
global constexpr float c_jump_strength = 6.75f;
global constexpr float c_max_gravity = 9.0f;
global constexpr float c_movement_speed = 3;
global constexpr float c_increased_movement_speed_per_upgrade = 0.1f;
global constexpr s_v2 c_player_size_v = {32, 32};
global constexpr s_v2 c_fist_size_v = {c_player_size_v.x * 0.4f, c_player_size_v.y * 0.4f};
global constexpr int c_map_version = 1;
global constexpr s_v2i c_atlas_size_v = {2679, 651};
global constexpr int c_atlas_padding = 1;
global constexpr int c_atlas_sprite_size = 13;
global constexpr int c_seconds_saved_per_upgrade = 3;
global constexpr float c_extra_loop_time_per_upgrade = 2;
global constexpr float c_loop_time = 20;
global constexpr float c_teleport_distance = 150;
global constexpr float c_super_speed_duration = 5;
global constexpr float c_super_speed_multiplier = 3;
global constexpr float c_shake_duration = 0.15f;
global constexpr float c_teleport_cooldown = 0.5f;
global constexpr int c_max_ghosts = 64;
global constexpr int c_max_ghost_positions = 120 * c_updates_per_second;
global constexpr int c_music_volume = 8;
global constexpr float c_circle_radius = 320;
global constexpr float c_player_attack_range = 64;
global constexpr s_v2 c_game_area = {c_world_size.x * 0.75f, c_world_size.y * 1.0f};
global constexpr float c_spawn_delay = 2;
global constexpr float c_dash_duration = 0.25f;
global constexpr float c_dash_cooldown = 3;
global constexpr int c_max_lives = 10;
global constexpr float c_enemy_health = 10;

global constexpr float c_game_speed_arr[] = {
	0.0f, 0.01f, 0.1f, 0.25f, 0.5f,
	1,
	2, 4, 8, 16
};


global constexpr int c_max_keys = 1024;
global constexpr int c_left_button = c_max_keys - 2;
global constexpr int c_right_button = c_max_keys - 1;
global constexpr int c_left_shift = c_max_keys - 3;

global constexpr int c_max_leaderboard_entries = 16;

global constexpr float c_pre_victory_duration = 2.0f;

global constexpr int c_leaderboard_id = 31492;

global constexpr s_len_str c_game_name = S("IDK");
