
global constexpr int c_max_hot_files = 64;

struct s_loaded_sound
{
	u8* data;
	u32 size_in_bytes;
	SDL_AudioSpec spec;
};

struct s_audio_fade
{
	float percent[2];
	float volume[2];
};

struct s_play_sound_data
{
	float volume = 1;
	float speed = 1;
	b8 loop;
	s_maybe<s_audio_fade> fade;
};

enum e_sound
{
	e_sound_click,
	e_sound_key,
	e_sound_break,
	e_sound_enemy_death,
	e_sound_enemy_death2,
	e_sound_clap,
	e_sound_land,
	e_sound_restart,
	e_sound_super_speed,
	e_sound_dash,
	e_sound_miss_attack,
	e_sound_punch,
	e_sound_lightning_bolt,
	e_sound_upgrade,
	e_sound_count,
};

struct s_sound_data
{
	char* path;
	float volume;
};

struct s_active_sound
{
	e_sound loaded_sound_id;
	s_play_sound_data data;
	float index;
};


global constexpr s_sound_data c_sound_data_arr[e_sound_count] = {
	{"assets/click.wav", 0.2f},
	{"assets/key.wav", 1.0f},
	{"assets/break.wav", 1.0f},
	{"assets/enemy_death.wav", 0.15f},
	{"assets/enemy_death2.wav", 0.15f},
	{"assets/clap.wav", 1.0f},
	{"assets/land.wav", 1.0f},
	{"assets/restart.wav", 1.0f},
	{"assets/super_speed.wav", 1.0f},
	{"assets/dash.wav", 1.0f},
	{"assets/miss_attack.wav", 0.1f},
	{"assets/punch.wav", 0.2f},
	{"assets/lightning_bolt.wav", 0.05f},
	{"assets/upgrade.wav", 0.2f},
};

struct s_platform_data
{
	b8 quit;
	b8 window_resized;
	u64 start_cycles;
	u64 cycle_frequency;
	SDL_Window* window;
	SDL_GLContext gl_context;
	s_v2i window_size;
	s_v2i prev_window_size;
	u8* memory;
	s_loaded_sound sound_arr[e_sound_count];

	s_list<s_active_sound, 128> active_sound_arr;

	s_loaded_sound (*load_sound_from_file)(char*);
	void (*play_sound)(e_sound, s_play_sound_data);

	#if defined(_WIN32) && defined(m_debug)
	int hot_read_index[2]; // @Note(tkap, 07/04/2025): One index for platform, other for game
	int hot_write_index;
	char hot_file_arr[c_max_hot_files][128];
	#endif
};

#if !(defined(_WIN32) && defined(m_debug))
void init(s_platform_data* platform_data);
void init_after_recompile(s_platform_data* platform_data);
void do_game(s_platform_data* platform_data);
#endif

#include "generated/generated_shared.cpp"