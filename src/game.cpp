
#define m_cpu_side 1

#pragma comment(lib, "opengl32.lib")

#pragma warning(push, 0)
// #define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"
#include "SDL_mixer.h"
#pragma warning(pop)

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/fetch.h>
#endif // __EMSCRIPTEN__

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#undef near
#undef far
#endif // _WIN32

#include <stdlib.h>

#include <gl/GL.h>
#if !defined(__EMSCRIPTEN__)
#include "external/glcorearb.h"
#endif

#if defined(__EMSCRIPTEN__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-value"
#endif

#pragma warning(push, 0)
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ASSERT
#include "external/stb_image.h"

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_assert
#include "external/stb_truetype.h"
#pragma warning(pop)

#if defined(__EMSCRIPTEN__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-value"
#endif

#if defined(__EMSCRIPTEN__)
#define m_gl_single_channel GL_LUMINANCE
#else // __EMSCRIPTEN__
#define m_gl_single_channel GL_RED
#endif // __EMSCRIPTEN__


#if defined(m_debug)
#define gl(...) __VA_ARGS__; {int error = glGetError(); if(error != 0) { on_gl_error(#__VA_ARGS__, __FILE__, __LINE__, error); }}
#else // m_debug
#define gl(...) __VA_ARGS__
#endif // m_debug

#include "tklib.h"
#include "shared.h"

#include "config.h"
#include "leaderboard.h"
#include "engine.h"
#include "game.h"
#include "shader_shared.h"


#if defined(__EMSCRIPTEN__)
global constexpr b8 c_on_web = true;
#else
global constexpr b8 c_on_web = false;
#endif



global s_platform_data* g_platform_data;
global s_game* game;
global s_v2 g_mouse;
global b8 g_left_click;

#if defined(__EMSCRIPTEN__)
#include "leaderboard.cpp"
#endif

#include "engine.cpp"

m_dll_export void init(s_platform_data* platform_data)
{
	g_platform_data = platform_data;
	game = (s_game*)platform_data->memory;
	game->speed_index = 5;
	game->rng = make_rng(1234);
	game->reload_shaders = true;
	game->speed = 0;
	game->hover_over_upgrade_pauses_game = true;

	SDL_StartTextInput();

	set_state(&game->state0, e_game_state0_main_menu);
	game->do_hard_reset = true;
	set_state(&game->hard_data.state1, e_game_state1_default);

	u8* cursor = platform_data->memory + sizeof(s_game);

	{
		game->arena = make_arena_from_memory(cursor, 10 * c_mb);
		cursor += 10 * c_mb;
	}
	{
		game->update_frame_arena = make_arena_from_memory(cursor, 10 * c_mb);
		cursor += 10 * c_mb;
	}
	{
		game->render_frame_arena = make_arena_from_memory(cursor, 500 * c_mb);
		cursor += 10 * c_mb;
	}
	{
		game->circular_arena = make_circular_arena_from_memory(cursor, 10 * c_mb);
		cursor += 10 * c_mb;
	}

	platform_data->cycle_frequency = SDL_GetPerformanceFrequency();
	platform_data->start_cycles = SDL_GetPerformanceCounter();

	platform_data->window_size.x = (int)c_world_size.x;
	platform_data->window_size.y = (int)c_world_size.y;

	g_platform_data->window = SDL_CreateWindow(
		"Loopscape", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
		(int)c_world_size.x, (int)c_world_size.y, SDL_WINDOW_HIDDEN | SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
	);
	SDL_SetWindowPosition(g_platform_data->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_ShowWindow(g_platform_data->window);

	#if defined(__EMSCRIPTEN__)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
	#else
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
	#endif

	g_platform_data->gl_context = SDL_GL_CreateContext(g_platform_data->window);
	SDL_GL_SetSwapInterval(1);

	#define X(type, name) name = (type)SDL_GL_GetProcAddress(#name); assert(name);
		m_gl_funcs
	#undef X

	{
		gl(glGenBuffers(1, &game->ubo));
		gl(glBindBuffer(GL_UNIFORM_BUFFER, game->ubo));
		gl(glBufferData(GL_UNIFORM_BUFFER, sizeof(s_uniform_data), NULL, GL_DYNAMIC_DRAW));
		gl(glBindBufferBase(GL_UNIFORM_BUFFER, 0, game->ubo));
	}

	{
		constexpr float c_size = 0.5f;
		s_vertex vertex_arr[] = {
			{v3(-c_size, -c_size, 0), v3(0, -1, 0), make_color(1), v2(0, 1)},
			{v3(c_size, -c_size, 0), v3(0, -1, 0), make_color(1), v2(1, 1)},
			{v3(c_size, c_size, 0), v3(0, -1, 0), make_color(1), v2(1, 0)},
			{v3(-c_size, -c_size, 0), v3(0, -1, 0), make_color(1), v2(0, 1)},
			{v3(c_size, c_size, 0), v3(0, -1, 0), make_color(1), v2(1, 0)},
			{v3(-c_size, c_size, 0), v3(0, -1, 0), make_color(1), v2(0, 0)},
		};
		game->mesh_arr[e_mesh_quad] = make_mesh_from_vertices(vertex_arr, array_count(vertex_arr));
	}

	{
		game->mesh_arr[e_mesh_cube] = make_mesh_from_obj_file("assets/cube.obj", &game->render_frame_arena);
		game->mesh_arr[e_mesh_sphere] = make_mesh_from_obj_file("assets/sphere.obj", &game->render_frame_arena);
	}

	{
		s_mesh* mesh = &game->mesh_arr[e_mesh_line];
		mesh->vertex_count = 6;
		gl(glGenVertexArrays(1, &mesh->vao));
		gl(glBindVertexArray(mesh->vao));

		gl(glGenBuffers(1, &mesh->instance_vbo.id));
		gl(glBindBuffer(GL_ARRAY_BUFFER, mesh->instance_vbo.id));

		u8* offset = 0;
		constexpr int stride = sizeof(float) * 9;
		int attrib_index = 0;

		gl(glVertexAttribPointer(attrib_index, 2, GL_FLOAT, GL_FALSE, stride, offset)); // line start
		gl(glEnableVertexAttribArray(attrib_index));
		gl(glVertexAttribDivisor(attrib_index, 1));
		attrib_index += 1;
		offset += sizeof(float) * 2;

		gl(glVertexAttribPointer(attrib_index, 2, GL_FLOAT, GL_FALSE, stride, offset)); // line end
		gl(glEnableVertexAttribArray(attrib_index));
		gl(glVertexAttribDivisor(attrib_index, 1));
		attrib_index += 1;
		offset += sizeof(float) * 2;

		gl(glVertexAttribPointer(attrib_index, 1, GL_FLOAT, GL_FALSE, stride, offset)); // line width
		gl(glEnableVertexAttribArray(attrib_index));
		gl(glVertexAttribDivisor(attrib_index, 1));
		attrib_index += 1;
		offset += sizeof(float) * 1;

		gl(glVertexAttribPointer(attrib_index, 4, GL_FLOAT, GL_FALSE, stride, offset)); // instance color
		gl(glEnableVertexAttribArray(attrib_index));
		gl(glVertexAttribDivisor(attrib_index, 1));
		attrib_index += 1;
		offset += sizeof(float) * 4;
	}

	for(int i = 0; i < e_sound_count; i += 1) {
		platform_data->sound_arr[i] = platform_data->load_sound_from_file(c_sound_data_arr[i].path);
	}

	Mix_Music* music = Mix_LoadMUS("assets/music.ogg");
	if(music) {
		Mix_VolumeMusic(c_music_volume);
		// Mix_PlayMusic(music, -1);
	}

	for(int i = 0; i < e_texture_count; i += 1) {
		char* path = c_texture_path_arr[i];
		if(strlen(path) > 0) {
			game->texture_arr[i] = load_texture_from_file(path, GL_NEAREST);
		}
	}

	game->font = load_font_from_file("assets/Inconsolata-Regular.ttf", 128, &game->render_frame_arena);

	{
		u32* texture = &game->texture_arr[e_texture_light].id;
		game->light_fbo.size.x = (int)c_world_size.x;
		game->light_fbo.size.y = (int)c_world_size.y;
		gl(glGenFramebuffers(1, &game->light_fbo.id));
		bind_framebuffer(game->light_fbo.id);
		gl(glGenTextures(1, texture));
		gl(glBindTexture(GL_TEXTURE_2D, *texture));
		gl(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, game->light_fbo.size.x, game->light_fbo.size.y, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL));
		gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
		gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
		gl(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, *texture, 0));
		assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
		bind_framebuffer(0);
	}

	#if defined(__EMSCRIPTEN__)
	load_or_create_leaderboard_id();
	#endif
}

m_dll_export void init_after_recompile(s_platform_data* platform_data)
{
	g_platform_data = platform_data;
	game = (s_game*)platform_data->memory;

	#define X(type, name) name = (type)SDL_GL_GetProcAddress(#name); assert(name);
		m_gl_funcs
	#undef X
}

#if defined(__EMSCRIPTEN__)
EM_JS(int, browser_get_width, (), {
	const { width, height } = canvas.getBoundingClientRect();
	return width;
});

EM_JS(int, browser_get_height, (), {
	const { width, height } = canvas.getBoundingClientRect();
	return height;
});
#endif // __EMSCRIPTEN__

m_dll_export void do_game(s_platform_data* platform_data)
{
	g_platform_data = platform_data;
	game = (s_game*)platform_data->memory;

	f64 delta64 = get_seconds() - game->time_before;
	game->time_before = get_seconds();

	{
		#if defined(__EMSCRIPTEN__)
		int width = browser_get_width();
		int height = browser_get_height();
		g_platform_data->window_size.x = width;
		g_platform_data->window_size.y = height;
		if(g_platform_data->prev_window_size.x != width || g_platform_data->prev_window_size.y != height) {
			set_window_size(width, height);
			g_platform_data->window_resized = true;
		}
		g_platform_data->prev_window_size.x = width;
		g_platform_data->prev_window_size.y = height;
		#endif // __EMSCRIPTEN__
	}

	// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		handle state start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
	{
		b8 state_changed = handle_state(&game->state0, game->render_time);
		if(state_changed) {
			game->accumulator += c_update_delay;
		}
	}
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		handle state end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

	input();
	float game_speed = c_game_speed_arr[game->speed_index] * game->speed;
	game->accumulator += delta64 * game_speed;
	#if defined(__EMSCRIPTEN__)
	game->accumulator = at_most(game->accumulator, 1.0);
	#else
	#endif
	while(game->accumulator >= c_update_delay) {
		game->accumulator -= c_update_delay;
		update();
	}
	float interp_dt = (float)(game->accumulator / c_update_delay);
	render(interp_dt, (float)delta64);
}

func void input()
{

	game->char_events.count = 0;
	game->key_events.count = 0;

	// u8* keyboard_state = (u8*)SDL_GetKeyboardState(null);
	// game->input.left = game->input.left || keyboard_state[SDL_SCANCODE_A];
	// game->input.right = game->input.right || keyboard_state[SDL_SCANCODE_D];


	for(int i = 0; i < c_max_keys; i += 1) {
		game->input_arr[i].half_transition_count = 0;
	}

	g_left_click = false;
	{
		int x;
		int y;
		SDL_GetMouseState(&x, &y);
		g_mouse = v2(x, y);
		s_rect letterbox = do_letterbox(v2(g_platform_data->window_size), c_world_size);
		g_mouse.x = range_lerp(g_mouse.x, letterbox.x, letterbox.x + letterbox.size.x, 0, c_world_size.x);
		g_mouse.y = range_lerp(g_mouse.y, letterbox.y, letterbox.y + letterbox.size.y, 0, c_world_size.y);
	}

	s_hard_game_data* hard_data = &game->hard_data;
	s_soft_game_data* soft_data = &game->soft_data;

	e_game_state0 state0 = (e_game_state0)get_state(&game->state0);
	e_game_state1 state1 = (e_game_state1)get_state(&hard_data->state1);

	SDL_Event event;
	while(SDL_PollEvent(&event) != 0) {
		switch(event.type) {
			case SDL_QUIT: {
				g_platform_data->quit = true;
			} break;

			case SDL_WINDOWEVENT: {
				if(event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
					int width = event.window.data1;
					int height = event.window.data2;
					g_platform_data->window_size.x = width;
					g_platform_data->window_size.y = height;
					g_platform_data->window_resized = true;
				}
			} break;

			case SDL_KEYDOWN:
			case SDL_KEYUP: {
				int key = event.key.keysym.sym;
				SDL_Scancode scancode = event.key.keysym.scancode;
				if(key == SDLK_LSHIFT) {
					key = c_left_shift;
				}
				else if(key == SDLK_LCTRL) {
					key = c_left_ctrl;
				}
				b8 is_down = event.type == SDL_KEYDOWN;
				handle_key_event(key, is_down, event.key.repeat != 0);

				if(key < c_max_keys) {
					game->input_arr[key].is_down = is_down;
					game->input_arr[key].half_transition_count += 1;
				}
				if(event.type == SDL_KEYDOWN) {
					if(key == SDLK_r && event.key.repeat == 0) {
						if(event.key.keysym.mod & KMOD_LCTRL) {
							game->do_hard_reset = true;
						}
						else if(state1 == e_game_state1_defeat) {
							game->do_hard_reset = true;
						}
					}
					else if(key == SDLK_SPACE && event.key.repeat == 0) {
					}
					else if(scancode == SDL_SCANCODE_A && event.key.repeat == 0) {
						soft_data->dash_timer.want_to_use_timestamp = game->update_time;
					}
					else if(scancode == SDL_SCANCODE_S && event.key.repeat == 0) {
						soft_data->attack_timer.want_to_use_timestamp = game->update_time;
					}
					else if(key == SDLK_ESCAPE && event.key.repeat == 0) {
						if(state0 == e_game_state0_play && state1 == e_game_state1_default) {
							add_state(&game->state0, e_game_state0_pause);
						}
						else if(state0 == e_game_state0_pause) {
							pop_state_transition(&game->state0, game->render_time, c_transition_time);
						}
					}
					#if defined(m_debug)
					else if(key == SDLK_s && event.key.repeat == 0 && event.key.keysym.mod & KMOD_LCTRL) {
					}
					else if(key == SDLK_l && event.key.repeat == 0 && event.key.keysym.mod & KMOD_LCTRL) {
					}
					else if(key == SDLK_KP_PLUS) {
						game->speed_index = circular_index(game->speed_index + 1, array_count(c_game_speed_arr));
						printf("Game speed: %f\n", c_game_speed_arr[game->speed_index]);
					}
					else if(key == SDLK_KP_MINUS) {
						game->speed_index = circular_index(game->speed_index - 1, array_count(c_game_speed_arr));
						printf("Game speed: %f\n", c_game_speed_arr[game->speed_index]);
					}
					else if(key == SDLK_F1) {
						game->cheat_menu_enabled = !game->cheat_menu_enabled;
					}
					else if(key == SDLK_j && event.key.repeat == 0) {
					}
					else if(key == SDLK_h && event.key.repeat == 0) {
					}
					#endif // m_debug

				}

				// @Note(tkap, 28/06/2025): Key up
				else {
					if(key == SDLK_SPACE) {
					}
				}
			} break;

			case SDL_MOUSEBUTTONDOWN:
			case SDL_MOUSEBUTTONUP:
			{
				if(event.type == SDL_MOUSEBUTTONDOWN && event.button.button == 1) {
					g_left_click = true;
					if(!are_we_hovering_over_ui(g_mouse)) {
						soft_data->attack_timer.want_to_use_timestamp = game->update_time;
					}
				}
				if(event.type == SDL_MOUSEBUTTONDOWN && event.button.button == 3) {
					if(!are_we_hovering_over_ui(g_mouse)) {
						soft_data->dash_timer.want_to_use_timestamp = game->update_time;
					}
				}
				// int key = sdl_key_to_windows_key(event.button.button);
				// b8 is_down = event.type == SDL_MOUSEBUTTONDOWN;
				// handle_key_event(key, is_down, false);

				if(event.button.button == 1) {
					int key = c_left_button;
					game->input_arr[key].is_down = event.type == SDL_MOUSEBUTTONDOWN;
					game->input_arr[key].half_transition_count += 1;
				}

				else if(event.button.button == 3) {
					int key = c_right_button;
					game->input_arr[key].is_down = event.type == SDL_MOUSEBUTTONDOWN;
					game->input_arr[key].half_transition_count += 1;
				}

			} break;

			case SDL_TEXTINPUT: {
				char c = event.text.text[0];
				game->char_events.add((char)c);
			} break;

			case SDL_MOUSEWHEEL: {
			} break;
		}
	}
}

func void update()
{
	game->update_frame_arena.used = 0;

	e_game_state0 state0 = (e_game_state0)get_state(&game->state0);

	s_hard_game_data* hard_data = &game->hard_data;
	s_soft_game_data* soft_data = &game->soft_data;
	auto entity_arr = &soft_data->entity_arr;

	float delta = (float)c_update_delay;

	if(game->do_hard_reset) {
		game->do_hard_reset = false;
		memset(hard_data, 0, sizeof(*hard_data));
		memset(soft_data, 0, sizeof(*soft_data));
		set_state(&game->hard_data.state1, e_game_state1_default);
		game->arena.used = 0;
		for_enum(type_i, e_entity) {
			entity_manager_reset(entity_arr, type_i);
		}

		soft_data->spawn_timer += c_spawn_delay;

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		create player start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		{
			s_entity player = zero;

			s_v2 center = gxy(0.5f);
			center.x += cosf(player.timer) * c_circle_radius * 0.5f;
			center.y += sinf(player.timer) * c_circle_radius * 0.5f;
			teleport_entity(&player, center);

			{
				s_entity emitter = zero;
				emitter.emitter_a = make_emitter_a();
				emitter.emitter_a.pos = v3(player.pos, 0.0f);
				emitter.emitter_a.radius = 4;
				emitter.emitter_a.follow_emitter = true;
				emitter.emitter_b = make_emitter_b();
				emitter.emitter_b.spawn_type = e_emitter_spawn_type_circle_outline;
				emitter.emitter_b.spawn_data.x = get_player_attack_range();
				emitter.emitter_b.duration = -1;
				emitter.emitter_b.particles_per_second = 100;
				emitter.emitter_b.particle_count = 1;
				player.range_emitter = add_emitter(emitter);
			}

			entity_manager_add(entity_arr, e_entity_player, player);
		}
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		create player end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		{
			s_entity emitter = make_circle_particles();
			add_emitter(emitter);
		}
	}
	if(game->do_soft_reset) {
	}

	b8 do_game = false;
	switch(state0) {
		case e_game_state0_play: {
			e_game_state1 state1 = (e_game_state1)get_state(&hard_data->state1);
			switch(state1) {
				xcase e_game_state1_default: {
					do_game = true;
				}
				xcase e_game_state1_defeat: {
				}
			}
		} break;

		default: {}
	}

	for(int i = 0; i < c_max_entities; i += 1) {
		if(entity_arr->active[i]) {
			entity_arr->data[i].prev_pos = entity_arr->data[i].pos;
		}
	}

	if(do_game) {
		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		spawn start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		{
			soft_data->spawn_timer += delta;
			while(soft_data->spawn_timer >= c_spawn_delay) {
				soft_data->spawn_timer -= c_spawn_delay;

				s_list<e_enemy, e_enemy_count> possible_spawn_arr;
				possible_spawn_arr.count = 0;
				for_enum(type_i, e_enemy) {
					b8 valid = type_i == 0;
					if(type_i > 0) {
						if(soft_data->enemy_type_kill_count_arr[type_i - 1] >= g_enemy_type_data[type_i].prev_enemy_required_kill_count) {
							valid = true;
						}
					}
					if(type_i == e_enemy_boss && soft_data->boss_spawned) {
						valid = false;
					}
					if(valid) {
						possible_spawn_arr.add(type_i);
					}
				}

				s_list<u32, e_enemy_count> weight_arr;
				weight_arr.count = 0;
				foreach_val(possible_i, possible, possible_spawn_arr) {
					weight_arr.add(g_enemy_type_data[possible].spawn_weight);
				}

				int chosen_index = pick_rand_from_weight_arr(&weight_arr, &game->rng);
				e_enemy chosen_enemy_type = possible_spawn_arr[chosen_index];

				if(chosen_enemy_type == e_enemy_boss) {
					soft_data->boss_spawned = true;
				}

				spawn_enemy(chosen_enemy_type);
			}
		}
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		spawn end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		update enemies start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		for(int i = c_first_index[e_entity_enemy]; i < c_last_index_plus_one[e_entity_enemy]; i += 1) {
			if(!entity_arr->active[i]) { continue; }
			s_entity* enemy = &entity_arr->data[i];
			s_enemy_type_data enemy_type_data = g_enemy_type_data[enemy->enemy_type];
			enemy->highlight = zero;
			s_v2 dir = v2_dir_from_to(enemy->pos, gxy(0.5f));
			if(enemy->knockback.valid) {
				enemy->pos += dir * -1 * enemy->knockback.value * delta;
				enemy->knockback.value *= 0.9f;
				if(enemy->knockback.value < 1.0f) {
					enemy->knockback = zero;
				}
			}
			else {
				float speed = 13 * enemy_type_data.speed_multi;
				float dist_before_moving = v2_distance(enemy->pos, gxy(0.5f));
				{
					float limit = c_circle_radius * 1.0f;
					float t = smoothstep(limit, limit + 50, dist_before_moving);
					speed += t * 300;
				}
				switch(enemy_type_data.movement_type) {
					xcase e_enemy_movement_zig_zag: {
						dir = v2_rotated(dir, sinf(game->update_time - enemy->spawn_timestamp) * c_pi * 0.4f);
					}
					xcase e_enemy_movement_spiral: {
						float t = smoothstep(0.0f, c_circle_radius * 2, dist_before_moving);
						dir = v2_rotated(dir, c_pi * 0.45f * powf(t, 0.25f));
					}
					break; default: {}
				}
				enemy->pos += dir * speed * delta;
				float dist = v2_distance(enemy->pos, gxy(0.5f));
				if(dist <= 10) {
					if(enemy->enemy_type == e_enemy_boss) {
						lose_lives(99999);
					}
					else {
						lose_lives(1);
					}
					add_emitter(make_lose_lives_particles());
					entity_manager_remove(entity_arr, e_entity_enemy, i);
				}
			}
		}
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		update enemies end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		update dying enemies start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		for(int i = c_first_index[e_entity_dying_enemy]; i < c_last_index_plus_one[e_entity_dying_enemy]; i += 1) {
			if(!entity_arr->active[i]) { continue; }
			s_entity* enemy = &entity_arr->data[i];
			s_v2 dir = v2_dir_from_to(enemy->pos, gxy(0.5f));
			enemy->pos += dir * -1 * enemy->knockback.value * delta;
			enemy->knockback.value *= 0.9f;
			if(enemy->knockback.value < 1.0f && enemy->remove_soon_timestamp <= 0) {
				enemy->remove_soon_timestamp = game->update_time;
			}
			if(enemy->remove_soon_timestamp > 0) {
				s_time_data time_data = get_time_data(game->update_time, enemy->remove_soon_timestamp, 0.25f);
				if(time_data.percent >= 1) {
					entity_manager_remove(entity_arr, e_entity_dying_enemy, i);
					play_sound(e_sound_enemy_death2, {.speed = get_rand_sound_speed(1.1f, &game->rng)});
					s_entity emitter = make_enemy_death_particles(enemy->pos);
					add_emitter(emitter);
				}
			}
		}
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		update dying enemies end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		update player start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		{
			s_entity* player = &soft_data->entity_arr.data[0];
			s_v2 center = gxy(0.5f);
			player->pos.x = cosf(player->timer) * c_circle_radius * 0.5f;
			player->pos.y = sinf(player->timer) * c_circle_radius * 0.5f;
			player->pos += center;
			if(timer_can_and_want_activate(soft_data->dash_timer, game->update_time, c_dash_duration, get_dash_cooldown(), 0.1f)) {
				play_sound(e_sound_dash, {.volume = 0.1f});
				timer_activate(&soft_data->dash_timer, game->update_time);
				if(completed_attack_tutorial()) {
					game->num_times_we_dashed += 1;
				}
			}

			float dash_speed = 1.0f;
			{
				b8 active = false;
				s_time_data time_data = timer_get_time_data(soft_data->dash_timer, game->update_time, c_dash_duration, &active);
				if(active) {
					dash_speed = 1.0f + time_data.inv_percent * 5;
				}
			}
			player->timer += delta * get_player_speed() * dash_speed;


			s_list<int, get_max_entities_of_type(e_entity_enemy)> enemy_index_arr;
			enemy_index_arr.count = 0;
			for(int i = c_first_index[e_entity_enemy]; i < c_last_index_plus_one[e_entity_enemy]; i += 1) {
				if(!entity_arr->active[i]) { continue; }
				enemy_index_arr.add(i);
			}
			radix_sort_32(enemy_index_arr.data, enemy_index_arr.count, get_radix_from_enemy_index, &game->update_frame_arena);

			// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		try to hit start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
			if(do_we_have_auto_attack()) {
				soft_data->attack_timer.want_to_use_timestamp = game->update_time;
			}
			int num_enemies_hit = 0;
			int num_possible_hits = get_hits_per_attack();
			b8 was_there_an_enemy_in_range = false;
			float attack_range = get_player_attack_range();
			float lightning_bolt_range = attack_range * 2;
			soft_data->lightning_bolt_timer.want_to_use_timestamp = game->update_time;
			b8 should_attack = timer_can_and_want_activate(soft_data->attack_timer, game->update_time, 0.0f, get_attack_or_auto_attack_cooldown(), 0.1f);
			b8 can_lightning_bolt = get_upgrade_level(e_upgrade_lightning_bolt) > 0 &&
				timer_can_and_want_activate(soft_data->lightning_bolt_timer, game->update_time, 0.0f, get_lightning_bolt_cooldown(), 0.0f);
			foreach_val(enemy_index_i, enemy_index, enemy_index_arr) {
				int i = enemy_index;
				if(num_enemies_hit >= num_possible_hits) { break; }
				if(!entity_arr->active[i]) { continue; }
				s_entity* enemy = &entity_arr->data[i];
				s_v2 enemy_size = get_enemy_size(enemy->enemy_type);

				float dist = v2_distance(enemy->pos, player->pos);
				float range = attack_range;
				if(can_lightning_bolt) {
					range = lightning_bolt_range;
				}
				if(dist <= range + enemy_size.y * 0.5f) {
					enemy->highlight = maybe(make_color(1, 0.6f, 0.6f));
					if(should_attack || can_lightning_bolt) {
						was_there_an_enemy_in_range = true;
						float knockback_multi = 0;
						float damage_multi = 0;
						if(can_lightning_bolt) {
							damage_multi = 0.5f;
							knockback_multi = 0.1f;
							can_lightning_bolt = false;
							timer_activate(&soft_data->lightning_bolt_timer, game->update_time);
							{
								s_audio_fade fade = make_simple_fade(0.8f, 1.0f);
								play_sound(e_sound_lightning_bolt, {.speed = get_rand_sound_speed(1.1f, &game->rng), .fade = maybe(fade)});
							}

							{
								s_entity effect = zero;
								effect.pos = enemy->pos;
								effect.spawn_timestamp = game->render_time;
								effect.effect_size = enemy_size;
								entity_manager_add(entity_arr, e_entity_visual_effect, effect);
							}
						}
						else {
							game->num_times_we_attacked_an_enemy += 1;
							damage_multi = 1;
							num_enemies_hit += 1;
							knockback_multi = 1;
							player->attacked_enemy_pos = enemy->pos;

							s_entity emitter = make_enemy_hit_particles(enemy->pos);
							add_emitter(emitter);

						}
						float knockback_to_add = get_player_knockback() * knockback_multi * (1.0f - g_enemy_type_data[enemy->enemy_type].knockback_resistance);
						if(enemy->knockback.valid) {
							enemy->knockback.value += knockback_to_add;
						}
						else {
							enemy->knockback = maybe(knockback_to_add);
						}

						b8 dead = damage_enemy(enemy, get_player_damage() * damage_multi);
						if(dead) {
							if(enemy->enemy_type == e_enemy_boss) {
								soft_data->frame_data.boss_defeated = true;
							}
							int gold_reward = g_enemy_type_data[enemy->enemy_type].gold_reward;
							{
								s_entity fct = zero;
								fct.duration = 0.66f;
								fct.fct_type = 1;
								fct.spawn_timestamp = game->render_time;
								builder_add(&fct.builder, "$$ffff00+%ig$.", gold_reward);
								fct.pos = enemy->pos;
								fct.pos.y += get_enemy_size(enemy->enemy_type).y * 0.5f;
								entity_manager_add(&game->soft_data.entity_arr, e_entity_fct, fct);
							}
							soft_data->spawn_timer += c_spawn_delay * 0.5f;
							soft_data->enemy_type_kill_count_arr[enemy->enemy_type] += 1;
							add_gold(gold_reward);
							make_dying_enemy(*enemy);
							entity_manager_remove(entity_arr, e_entity_enemy, i);
						}
					}
				}
			}
			// @Note(tkap, 01/08/2025): We pressed the attack key but there were no enemies in range
			float time_since_last_lightning_bolt = game->update_time - soft_data->lightning_bolt_timer.used_timestamp;
			b8 lightning_bolted_recently = time_since_last_lightning_bolt <= 0.33f && soft_data->lightning_bolt_timer.used_timestamp > 0;
			if(should_attack && !was_there_an_enemy_in_range && !lightning_bolted_recently && !do_we_have_auto_attack()) {
				timer_activate(&soft_data->attack_timer, game->update_time);
				play_sound(e_sound_miss_attack, zero);
			}
			if(num_enemies_hit > 0) {
				play_sound(e_sound_punch, {.speed = get_rand_sound_speed(1.1f, &game->rng)});
				if(do_we_have_auto_attack()) {
					timer_activate(&soft_data->attack_timer, game->update_time);
				}
				else {
					soft_data->attack_timer.want_to_use_timestamp = 0;
				}
				player->did_attack_enemy_timestamp = game->update_time;
			}
			// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		try to hit end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		}
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		update player end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		game->update_time += (float)c_update_delay;
		hard_data->update_count += 1;
		soft_data->update_count += 1;
	}

	if(soft_data->frame_data.lives_to_lose > 0) {
		game->soft_data.lives_lost += soft_data->frame_data.lives_to_lose;
		game->soft_data.life_change_timestamp = game->render_time;
		int curr_lives = get_max_lives() - game->soft_data.lives_lost;
		if(curr_lives <= 0) {
			add_state(&game->hard_data.state1, e_game_state1_defeat);
		}
	}
	if(soft_data->frame_data.boss_defeated) {
		if(game->leaderboard_nice_name.count <= 0 && c_on_web) {
			add_temporary_state_transition(&game->state0, e_game_state0_input_name, game->render_time, c_transition_time);
		}
		else if(!soft_data->tried_to_submit_score) {
			soft_data->tried_to_submit_score = true;
			add_state_transition(&game->state0, e_game_state0_win_leaderboard, game->render_time, c_transition_time);
			game->update_count_at_win_time = hard_data->update_count;

			#if defined(__EMSCRIPTEN__)
			submit_leaderboard_score(hard_data->update_count, c_leaderboard_id);
			#endif
		}
	}
	memset(&soft_data->frame_data, 0, sizeof(soft_data->frame_data));
}

func void render(float interp_dt, float delta)
{
	game->render_frame_arena.used = 0;

	if(!game->music_volume_clean) {
		game->music_volume_clean = true;
		if(game->disable_music) {
			Mix_VolumeMusic(0);
		}
		else {
			Mix_VolumeMusic(c_music_volume);
		}
	}

	#if defined(_WIN32)
	while(g_platform_data->hot_read_index[1] < g_platform_data->hot_write_index) {
		char* path = g_platform_data->hot_file_arr[g_platform_data->hot_read_index[1] % c_max_hot_files];
		if(strstr(path, ".shader")) {
			game->reload_shaders = true;
		}
		g_platform_data->hot_read_index[1] += 1;
	}
	#endif // _WIN32

	if(game->reload_shaders) {
		game->reload_shaders = false;

		for(int i = 0; i < e_shader_count; i += 1) {
			s_shader shader = load_shader_from_file(c_shader_path_arr[i], &game->render_frame_arena);
			if(shader.id > 0) {
				if(game->shader_arr[i].id > 0) {
					gl(glDeleteProgram(game->shader_arr[i].id));
				}
				game->shader_arr[i] = shader;

				#if defined(m_debug)
				printf("Loaded %s\n", c_shader_path_arr[i]);
				#endif // m_debug
			}
		}
	}

	for(int i = 0; i < e_shader_count; i += 1) {
		for(int j = 0; j < e_texture_count; j += 1) {
			for(int k = 0; k < e_mesh_count; k += 1) {
				game->render_group_index_arr[i][j][k] = -1;
			}
		}
	}
	game->render_group_arr.count = 0;
	memset(game->render_instance_count, 0, sizeof(game->render_instance_count));

	s_hard_game_data* hard_data = &game->hard_data;
	s_soft_game_data* soft_data = &game->soft_data;

	s_m4 ortho = make_orthographic(0, c_world_size.x, c_world_size.y, 0, -100, 100);

	bind_framebuffer(0);
	clear_framebuffer_depth(0);
	clear_framebuffer_color(0, v4(0.0f, 0, 0, 0));

	e_game_state0 state0 = (e_game_state0)get_state(&game->state0);

	b8 do_game = false;
	b8 do_defeat = false;

	switch(state0) {

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		main menu start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		case e_game_state0_main_menu: {
			game->speed = 0;

			draw_background(zero, ortho);

			if(do_button(S("Play"), wxy(0.5f, 0.5f), true) || is_key_pressed(SDLK_RETURN, true)) {
				add_state_transition(&game->state0, e_game_state0_play, game->render_time, c_transition_time);
				game->do_hard_reset = true;
			}

			if(do_button(S("Leaderboard"), wxy(0.5f, 0.6f), true)) {
				#if defined(__EMSCRIPTEN__)
				get_leaderboard(c_leaderboard_id);
				#endif
				add_state_transition(&game->state0, e_game_state0_leaderboard, game->render_time, c_transition_time);
			}

			if(do_button(S("Options"), wxy(0.5f, 0.7f), true)) {
				add_state_transition(&game->state0, e_game_state0_options, game->render_time, c_transition_time);
			}

			draw_text(c_game_name, wxy(0.5f, 0.2f), 128, make_color(1), true, &game->font, zero);
			draw_text(S("www.twitch.tv/Tkap1"), wxy(0.5f, 0.3f), 32, make_color(0.6f), true, &game->font, zero);

			{
				s_render_flush_data data = make_render_flush_data(zero, zero);
				data.projection = ortho;
				data.blend_mode = e_blend_mode_normal;
				data.depth_mode = e_depth_mode_no_read_yes_write;
				render_flush(data, true);
			}

		} break;
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		main menu end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		pause menu start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		case e_game_state0_pause: {
			game->speed = 0;

			draw_background(zero, ortho);

			if(do_button(S("Resume"), wxy(0.5f, 0.5f), true) || is_key_pressed(SDLK_RETURN, true)) {
				pop_state_transition(&game->state0, game->render_time, c_transition_time);
			}

			if(do_button(S("Leaderboard"), wxy(0.5f, 0.6f), true)) {
				#if defined(__EMSCRIPTEN__)
				get_leaderboard(c_leaderboard_id);
				#endif
				add_state_transition(&game->state0, e_game_state0_leaderboard, game->render_time, c_transition_time);
			}

			if(do_button(S("Options"), wxy(0.5f, 0.7f), true)) {
				add_state_transition(&game->state0, e_game_state0_options, game->render_time, c_transition_time);
			}

			draw_text(c_game_name, wxy(0.5f, 0.2f), 128, make_color(1), true, &game->font, zero);
			draw_text(S("www.twitch.tv/Tkap1"), wxy(0.5f, 0.3f), 32, make_color(0.6f), true, &game->font, zero);

			{
				s_render_flush_data data = make_render_flush_data(zero, zero);
				data.projection = ortho;
				data.blend_mode = e_blend_mode_normal;
				data.depth_mode = e_depth_mode_no_read_yes_write;
				render_flush(data, true);
			}

		} break;
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		pause menu end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		leaderboard start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		case e_game_state0_leaderboard: {
			game->speed = 0;
			draw_background(zero, ortho);
			do_leaderboard();

			{
				s_render_flush_data data = make_render_flush_data(zero, zero);
				data.projection = ortho;
				data.blend_mode = e_blend_mode_normal;
				data.depth_mode = e_depth_mode_no_read_yes_write;
				render_flush(data, true);
			}

		} break;

		case e_game_state0_win_leaderboard: {
			game->speed = 0;
			draw_background(zero, ortho);
			do_leaderboard();

			{
				s_time_format data = update_count_to_time_format(game->update_count_at_win_time);
				s_len_str text = format_text("%02i:%02i.%i", data.minutes, data.seconds, data.milliseconds);
				draw_text(text, c_world_center * v2(1.0f, 0.2f), 64, make_color(1), true, &game->font, zero);

				draw_text(S("Press R to restart..."), c_world_center * v2(1.0f, 0.4f), sin_range(48, 60, game->render_time * 8.0f), make_color(0.66f), true, &game->font, zero);
			}

			b8 want_to_reset = is_key_pressed(SDLK_r, true);
			if(
				do_button(S("Restart"), c_world_size * v2(0.87f, 0.82f), true)
				|| is_key_pressed(SDLK_ESCAPE, true) || want_to_reset
			) {
				pop_state_transition(&game->state0, game->render_time, c_transition_time);
				game->do_hard_reset = true;
			}

			{
				s_render_flush_data data = make_render_flush_data(zero, zero);
				data.projection = ortho;
				data.blend_mode = e_blend_mode_normal;
				data.depth_mode = e_depth_mode_no_read_yes_write;
				render_flush(data, true);
			}

		} break;
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		leaderboard end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		options start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		case e_game_state0_options: {
			game->speed = 0;
			draw_background(zero, ortho);

			s_v2 pos = wxy(0.5f, 0.2f);
			s_v2 button_size = v2(600, 48);

			{
				s_len_str text = format_text("Sound: %s", game->turn_off_all_sounds ? "Off" : "On");
				do_bool_button_ex(text, pos, button_size, true, &game->turn_off_all_sounds);
				pos.y += 80;
			}

			{
				s_len_str text = format_text("Music: %s", game->disable_music ? "Off" : "On");
				b8 result = do_bool_button_ex(text, pos, button_size, true, &game->disable_music);
				if(result) {
					game->music_volume_clean = false;
				}
				pos.y += 80;
			}

			{
				s_len_str text = format_text("Replay ghosts: %s", game->hide_ghosts ? "Off" : "On");
				do_bool_button_ex(text, pos, button_size, true, &game->hide_ghosts);
				pos.y += 80;
			}

			{
				s_len_str text = format_text("Background: %s", game->hide_background ? "Off" : "On");
				do_bool_button_ex(text, pos, button_size, true, &game->hide_background);
				pos.y += 80;
			}

			{
				s_len_str text = format_text("Screen shake: %s", game->disable_screen_shake ? "Off" : "On");
				do_bool_button_ex(text, pos, button_size, true, &game->disable_screen_shake);
				pos.y += 80;
			}

			{
				s_len_str text = format_text("Show timer: %s", game->hide_timer ? "Off" : "On");
				do_bool_button_ex(text, pos, button_size, true, &game->hide_timer);
				pos.y += 80;
			}

			{
				s_len_str text = format_text("Dim player when out of jumps: %s", game->dim_player_when_out_of_jumps ? "On" : "Off");
				do_bool_button_ex(text, pos, button_size, true, &game->dim_player_when_out_of_jumps);
				pos.y += 80;
			}

			b8 escape = is_key_pressed(SDLK_ESCAPE, true);
			if(do_button(S("Back"), wxy(0.87f, 0.92f), true) || escape) {
				pop_state_transition(&game->state0, game->render_time, c_transition_time);
			}

			{
				s_render_flush_data data = make_render_flush_data(zero, zero);
				data.projection = ortho;
				data.blend_mode = e_blend_mode_normal;
				data.depth_mode = e_depth_mode_no_read_yes_write;
				render_flush(data, true);
			}
		} break;
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		options end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		play start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		case e_game_state0_play: {
			game->speed = 1;


			handle_state(&hard_data->state1, game->render_time);

			e_game_state1 state1 = (e_game_state1)get_state(&hard_data->state1);
			switch(state1) {
				xcase e_game_state1_default: {
					do_game = true;
				}
				xcase e_game_state1_defeat: {
					do_game = true;
					do_defeat = true;
				}
			}

		} break;
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		play end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		case e_game_state0_input_name: {
			game->speed = 0;
			draw_background(zero, ortho);
			s_input_name_state* state = &game->input_name_state;
			float font_size = 36;
			s_v2 pos = c_world_size * v2(0.5f, 0.4f);

			int count_before = state->name.str.count;
			b8 submitted = handle_string_input(&state->name, game->render_time);
			int count_after = state->name.str.count;
			if(count_before != count_after) {
				play_sound(e_sound_key, zero);
			}
			if(submitted) {
				b8 can_submit = true;
				if(state->name.str.count < 2) {
					can_submit = false;
					cstr_into_builder(&state->error_str, "Name must have at least 2 characters!");
				}
				if(can_submit) {
					state->error_str.count = 0;
					#if defined(__EMSCRIPTEN__)
					set_leaderboard_name(builder_to_len_str(&state->name.str));
					#endif
					game->leaderboard_nice_name = state->name.str;
				}
			}

			draw_text(S("Victory!"), c_world_size * v2(0.5f, 0.1f), font_size, make_color(1), true, &game->font, zero);
			draw_text(S("Enter your name"), c_world_size * v2(0.5f, 0.2f), font_size, make_color(1), true, &game->font, zero);
			if(state->error_str.count > 0) {
				draw_text(builder_to_len_str(&state->error_str), c_world_size * v2(0.5f, 0.3f), font_size, hex_to_rgb(0xD77870), true, &game->font, zero);
			}

			if(state->name.str.count > 0) {
				draw_text(builder_to_len_str(&state->name.str), pos, font_size, make_color(1), true, &game->font, zero);
			}

			s_v2 full_text_size = get_text_size(builder_to_len_str(&state->name.str), &game->font, font_size);
			s_v2 partial_text_size = get_text_size_with_count(builder_to_len_str(&state->name.str), &game->font, font_size, state->name.cursor.value, 0);
			s_v2 cursor_pos = v2(
				-full_text_size.x * 0.5f + pos.x + partial_text_size.x,
				pos.y - font_size * 0.5f
			);

			s_v2 cursor_size = v2(15.0f, font_size);
			float t = game->render_time - max(state->name.last_action_time, state->name.last_edit_time);
			b8 blink = false;
			constexpr float c_blink_rate = 0.75f;
			if(t > 0.75f && fmodf(t, c_blink_rate) >= c_blink_rate / 2) {
				blink = true;
			}
			float t2 = clamp(game->render_time - state->name.last_edit_time, 0.0f, 1.0f);
			s_v4 color = lerp_color(hex_to_rgb(0xffdddd), multiply_rgb_clamped(hex_to_rgb(0xABC28F), 0.8f), 1 - powf(1 - t2, 3));
			float extra_height = ease_out_elastic2_advanced(t2, 0, 0.75f, 20, 0);
			cursor_size.y += extra_height;

			if(!state->name.visual_pos_initialized) {
				state->name.visual_pos_initialized = true;
				state->name.cursor_visual_pos = cursor_pos;
			}
			else {
				state->name.cursor_visual_pos = lerp_snap(state->name.cursor_visual_pos, cursor_pos, delta * 20);
			}

			if(!blink) {
				draw_rect_topleft(state->name.cursor_visual_pos - v2(0.0f, extra_height / 2), cursor_size, color);
			}

			s_render_flush_data data = make_render_flush_data(zero, zero);
			data.projection = ortho;
			data.blend_mode = e_blend_mode_normal;
			data.depth_mode = e_depth_mode_no_read_no_write;
			render_flush(data, true);

		} break;
	}

	if(do_game) {
		b8 do_game_ui = true;
		auto entity_arr = &soft_data->entity_arr;

		s_entity* player = &entity_arr->data[0];

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		tiles start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		{
			{
				s_instance_data data = zero;
				data.model = m4_translate(v3(gxy(0.5f), 0));
				data.model = m4_multiply(data.model, m4_scale(v3(c_game_area, 1)));
				data.color = make_color(1);
				add_to_render_group(data, e_shader_tile_background, e_texture_white, e_mesh_quad);
			}

			s_render_flush_data data = make_render_flush_data(zero, zero);
			data.projection = ortho;
			data.blend_mode = e_blend_mode_normal;
			data.depth_mode = e_depth_mode_no_read_no_write;
			render_flush(data, true);
		}
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		tiles end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		{
			s_v2 size = v2(32);
			draw_atlas(gxy(0.5f) + size * v2(-0.5f, -0.5f), v2(32), v2i(78, 16), make_color(1));
			draw_atlas(gxy(0.5f) + size * v2(0.5f, -0.5f), v2(32), v2i(79, 16), make_color(1));
			draw_atlas(gxy(0.5f) + size * v2(-0.5f, 0.5f), v2(32), v2i(78, 17), make_color(1));
			draw_atlas(gxy(0.5f) + size * v2(0.5f, 0.5f), v2(32), v2i(79, 17), make_color(1));
		}

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		draw enemies start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		{
			for(int i = c_first_index[e_entity_enemy]; i < c_last_index_plus_one[e_entity_enemy]; i += 1) {
				if(!entity_arr->active[i]) { continue; }
				s_entity* enemy = &entity_arr->data[i];
				s_v2 enemy_pos = lerp_v2(enemy->prev_pos, enemy->pos, interp_dt);
				s_v4 color = make_color(1);
				// if(enemy->highlight.valid) {
				// 	color = enemy->highlight.value;
				// }
				s_time_data time_data = get_time_data(update_time_to_render_time(game->update_time, interp_dt), enemy->hit_timestamp, 0.33f);
				s_draw_data draw_data = zero;
				draw_data.mix_color = make_color(1);
				if(enemy->hit_timestamp > 0 && time_data.percent <= 1) {
					draw_data.mix_weight = smoothstep(1.0f, 0.7f, time_data.percent);
				}
				s_v2 dir = v2_dir_from_to(enemy_pos, gxy(0.5f));
				if(dir.x > 0) {
					draw_data.flip_x = true;
				}
				s_v2 size = get_enemy_size(enemy->enemy_type);
				draw_atlas_ex(enemy_pos, size, get_enemy_atlas_index(enemy->enemy_type), color, 0, draw_data);
				s_v2 light_offset = v2(-8, 0);
				if(!draw_data.flip_x) {
					light_offset.x *= -1;
				}
				add_multiplicative_light(enemy_pos + light_offset, size.x * 2, make_color(0.5f), 0.0f);
			}
		}
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		draw enemies end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		draw dying enemies start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		{
			for(int i = c_first_index[e_entity_dying_enemy]; i < c_last_index_plus_one[e_entity_dying_enemy]; i += 1) {
				if(!entity_arr->active[i]) { continue; }
				s_entity* enemy = &entity_arr->data[i];
				s_v2 enemy_pos = lerp_v2(enemy->prev_pos, enemy->pos, interp_dt);
				// s_v4 color = enemy->highlight.value;
				s_v4 color = make_color(1);
				s_time_data time_data = get_time_data(update_time_to_render_time(game->update_time, interp_dt), enemy->hit_timestamp, 0.33f);
				s_draw_data draw_data = zero;
				draw_data.mix_color = make_color(1);
				if(enemy->hit_timestamp > 0 && time_data.percent <= 1) {
					draw_data.mix_weight = smoothstep(1.0f, 0.7f, time_data.percent);
				}
				draw_atlas_ex(enemy_pos, get_enemy_size(enemy->enemy_type), get_enemy_atlas_index(enemy->enemy_type), color, 0, draw_data);
				// add_additive_light(enemy_pos, 64, make_color(1), 0.0f);
			}
		}
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		draw dying enemies end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		s_v2 player_pos = lerp_v2(player->prev_pos, player->pos, interp_dt);
		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		draw player start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		{
			draw_atlas(player_pos, c_player_size_v, v2i(123, 42), make_color(1));
			entity_arr->data[player->range_emitter].emitter_a.pos = v3(player_pos, 0.0f);
			entity_arr->data[player->range_emitter].emitter_b.spawn_data.x = get_player_attack_range();

			// @Note(tkap, 31/07/2025): Fist
			{
				s_v2 offset_arr[] = {
					v2(-c_player_size_v.x, 0.0f),
					v2(c_player_size_v.x, 0.0f),
				};
				for(int i = 0; i < 2; i += 1) {
					s_v2 pos = player_pos + offset_arr[i];
					pos.y += sinf(player->fist_wobble_time * 4) * c_player_size_v.y * 0.1f;
					s_v2 size = c_fist_size_v;
					s_v4 color = make_color(1);
					float target_rotation = i == 0 ? 0.33f : -0.33f;

					if(player->did_attack_enemy_timestamp > 0) {
						target_rotation += v2_angle(player->attacked_enemy_pos - player_pos) + c_pi * 0.5f;
						float passed = update_time_to_render_time(game->update_time, interp_dt) - player->did_attack_enemy_timestamp;
						s_animator animator = zero;
						s_v2 temp_pos = zero;
						s_v2 temp_size = size;
						animate_v2(&animator, pos, player->attacked_enemy_pos, 0.1f, &temp_pos, e_ease_linear, passed);
						animate_v2(&animator, size, size * 2, 0.1f, &temp_size, e_ease_linear, passed);
						animate_color(&animator, make_color(1), make_color(1, 0, 0), 0.1f, &color, e_ease_linear, passed);
						animator_end_keyframe(&animator, 0.0f);
						animate_v2(&animator, player->attacked_enemy_pos, pos, 0.5f, &temp_pos, e_ease_out_elastic, passed);
						animate_v2(&animator, size * 2, size, 0.5f, &temp_size, e_ease_out_elastic, passed);
						animate_color(&animator, make_color(1, 0, 0), make_color(1), 0.5f, &color, e_ease_linear, passed);
						float time = 0;
						animate_float(&animator, 0.0f, 0.5f, 0.5f, &time, e_ease_linear, passed);
						if(time >= 0.5f) {
							player->did_attack_enemy_timestamp = false;
						}
						pos = temp_pos;
						size = temp_size;
					}
					else {
						player->fist_wobble_time += delta;
					}

					b8 flip_x = i == 1;
					player->fist_rotation[i] = lerp_angle(player->fist_rotation[i], target_rotation, at_most(1.0f, delta * 10));
					draw_atlas_ex(pos, size, v2i(122, 42), make_color(1), player->fist_rotation[i], {.flip_x = flip_x});
				}
			}
		}
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		draw player end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		{
			s_render_flush_data data = make_render_flush_data(zero, zero);
			data.projection = ortho;
			data.blend_mode = e_blend_mode_normal;
			data.depth_mode = e_depth_mode_no_read_no_write;
			render_flush(data, true);
		}

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		draw visual effects start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		for(int i = c_first_index[e_entity_visual_effect]; i < c_last_index_plus_one[e_entity_visual_effect]; i += 1) {
				if(!entity_arr->active[i]) { continue; }
				s_entity* effect = &entity_arr->data[i];
				s_time_data time_data = get_time_data(game->render_time, effect->spawn_timestamp, 1.0f);

				{
					s_v2 size = effect->effect_size * 2;
					float bottom = effect->pos.y + effect->effect_size.y * 0.5f;
					s_v2 pos = effect->pos;
					pos.y = bottom - size.y * 0.5f;
					s_instance_data data = zero;
					data.model = m4_translate(v3(pos, 0));
					data.model = m4_multiply(data.model, m4_scale(v3(size, 1)));
					float alpha = powf(time_data.inv_percent, 0.5f);
					data.color = make_color(1, alpha);
					add_to_render_group(data, e_shader_lightning, e_texture_white, e_mesh_quad);

					{
						s_v4 color = hex_to_rgb(0x0076FF);
						color = multiply_rgb(color, alpha * 0.75f);
						add_additive_light(pos, effect->effect_size.y * 2, color, 0.0f);
					}
				}


				if(time_data.percent >= 1) {
					entity_manager_remove(entity_arr, e_entity_visual_effect, i);
				}
		}
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		draw visual effects end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		update_particles(delta);

		{
			s_render_flush_data data = make_render_flush_data(zero, zero);
			data.projection = ortho;
			data.blend_mode = e_blend_mode_additive;
			data.depth_mode = e_depth_mode_no_read_no_write;
			render_flush(data, true);
		}

		if(cheat_key(SDLK_f)) {
			game->do_lights = !game->do_lights;
		}

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		multiplicative lights start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		if(game->do_lights) {

			clear_framebuffer_color(game->light_fbo.id, make_color(0.5f));

			draw_light(player_pos, 256, make_color(0.75f), 0.0f);
			foreach_val(light_i, light, game->multiplicative_light_arr) {
				draw_light(light.pos, light.radius, light.color, light.smoothness);
			}
			game->multiplicative_light_arr.count = 0;
			{
				s_render_flush_data data = make_render_flush_data(zero, zero);
				data.projection = ortho;
				data.blend_mode = e_blend_mode_additive;
				data.depth_mode = e_depth_mode_no_read_no_write;
				data.fbo = game->light_fbo;
				render_flush(data, true);
			}

			draw_texture_screen(c_world_center, c_world_size, make_color(1), e_texture_light, e_shader_flat, v2(0, 1), v2(1, 0), zero);
			{
				s_render_flush_data data = make_render_flush_data(zero, zero);
				data.projection = ortho;
				data.blend_mode = e_blend_mode_multiply;
				data.depth_mode = e_depth_mode_no_read_no_write;
				render_flush(data, true);
			}
		}
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		multiplicative lights end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		additive lights start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		if(game->do_lights) {

			clear_framebuffer_color(game->light_fbo.id, make_color(0.0f));

			draw_light(gxy(0.5f), 300, make_color(0.5f, 0.33f, 0.0f), 0.0f);
			draw_light(player_pos + v2(0, 8), 24, make_color(0.0f, 0.15f, 0.0f), 0.0f);

			foreach_val(light_i, light, game->additive_light_arr) {
				draw_light(light.pos, light.radius, light.color, light.smoothness);
			}
			game->additive_light_arr.count = 0;
			{
				s_render_flush_data data = make_render_flush_data(zero, zero);
				data.projection = ortho;
				data.blend_mode = e_blend_mode_additive;
				data.depth_mode = e_depth_mode_no_read_no_write;
				data.fbo = game->light_fbo;
				render_flush(data, true);
			}

			draw_texture_screen(c_world_center, c_world_size, make_color(1), e_texture_light, e_shader_flat, v2(0, 1), v2(1, 0), zero);
			{
				s_render_flush_data data = make_render_flush_data(zero, zero);
				data.projection = ortho;
				data.blend_mode = e_blend_mode_additive;
				data.depth_mode = e_depth_mode_no_read_no_write;
				render_flush(data, true);
			}
		}
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		additive lights end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		draw fct start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		for(int i = c_first_index[e_entity_fct]; i < c_last_index_plus_one[e_entity_fct]; i += 1) {
			if(!entity_arr->active[i]) { continue; }
			s_entity* fct = &entity_arr->data[i];
			if(fct->fct_type == 0) {
				s_v2 vel = v2(0, -400) + fct->vel;
				fct->vel.y += 4;
				fct->pos += vel * delta;
			}
			s_time_data time_data = get_time_data(game->render_time, fct->spawn_timestamp, fct->duration);
			{
				s_len_str str = builder_to_str(&fct->builder);
				s_v4 color = make_color(1);
				color.a = powf(time_data.inv_percent, 0.5f);
				draw_text(str, fct->pos, 32, color, true, &game->font, zero);
			}
			if(time_data.percent >= 1) {
				entity_manager_remove(entity_arr, e_entity_fct, i);
			}
		}
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		draw fct end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		{
			s_render_flush_data data = make_render_flush_data(zero, zero);
			data.projection = ortho;
			data.blend_mode = e_blend_mode_normal;
			data.depth_mode = e_depth_mode_no_read_no_write;
			render_flush(data, true);
		}

		{
			b8 do_flush = false;
			// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		draw attack cooldown start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
			{
				b8 is_on_cooldown = false;
				float time = update_time_to_render_time(game->update_time, interp_dt);
				s_time_data time_data = timer_get_cooldown_time_data(soft_data->attack_timer, time, 0.0f, get_attack_or_auto_attack_cooldown(), &is_on_cooldown);
				if(is_on_cooldown) {
					do_flush = true;
					s_v2 size = v2(64, 10);
					s_v2 pos = player_pos;
					pos.y += c_player_size_v.y * 0.5f;
					pos.y += size.y * 0.8f;
					s_v2 over_pos = pos;
					s_v2 over_size = size;
					over_size.x *= time_data.percent;
					over_pos.x -= size.x * 0.5f;
					over_pos.x += over_size.x * 0.5f;
					s_v4 color = hex_to_rgb(0x14A12C);
					color.a = ease_linear_advanced(time_data.percent, 0.9f, 1.0f, 1, 0.0f);
					draw_rect(pos, size, color);
					draw_rect(over_pos, over_size, multiply_rgb(color, 2));
				}
			}
			// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		draw attack cooldown end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

			// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		draw dash cooldown start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
			{
				b8 is_on_cooldown = false;
				float time = update_time_to_render_time(game->update_time, interp_dt);
				s_time_data time_data = timer_get_cooldown_time_data(soft_data->dash_timer, time, c_dash_duration, get_dash_cooldown(), &is_on_cooldown);
				if(is_on_cooldown && time_data.percent >= 0) {
					do_flush = true;
					s_v2 size = v2(64, 10);
					s_v2 pos = player_pos;
					pos.y += c_player_size_v.y * 0.5f;
					pos.y += size.y * 0.8f;
					pos.y += 16;
					s_v2 over_pos = pos;
					s_v2 over_size = size;
					over_size.x *= time_data.percent;
					over_pos.x -= size.x * 0.5f;
					over_pos.x += over_size.x * 0.5f;
					s_v4 color = hex_to_rgb(0x106BA8);
					color.a = ease_linear_advanced(time_data.percent, 0.9f, 1.0f, 1, 0.0f);
					draw_rect(pos, size, color);
					draw_rect(over_pos, over_size, multiply_rgb(color, 2));
				}
			}
			// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		draw dash cooldown end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

			if(do_flush) {
				s_render_flush_data data = make_render_flush_data(zero, zero);
				data.projection = ortho;
				data.blend_mode = e_blend_mode_normal;
				data.depth_mode = e_depth_mode_no_read_no_write;
				render_flush(data, true);
			}
		}

		if(do_game_ui) {

			s_rect rect = {
				c_game_area.x, 176.0f, c_world_size.x - c_game_area.x, c_world_size.y
			};
			s_v2 button_size = v2(320, 48);
			s_container container = make_down_center_x_container(rect, button_size, 10);

			{
				draw_rect_topleft(v2(rect.pos.x, 0.0f), v2(rect.size.x, c_world_size.y), make_color(0.05f));
				{
					s_render_flush_data data = make_render_flush_data(zero, zero);
					data.projection = ortho;
					data.blend_mode = e_blend_mode_normal;
					data.depth_mode = e_depth_mode_no_read_no_write;
					render_flush(data, true);
				}
			}

			for_enum(upgrade_i, e_upgrade) {
				s_upgrade_data data = g_upgrade_data[upgrade_i];
				int how_many_to_buy = 1;
				if(is_key_down(c_left_ctrl)) {
					how_many_to_buy *= 5;
				}
				if(is_key_down(c_left_shift)) {
					how_many_to_buy *= 10;
				}
				if(data.max_upgrades > 0) {
					int curr_level = get_upgrade_level(upgrade_i);
					int how_many_to_reach_max = data.max_upgrades - curr_level;
					how_many_to_buy = at_most(how_many_to_reach_max, how_many_to_buy);
				}
				s_v2 pos = container_get_pos_and_advance(&container);
				int cost_all = 0;
				int how_many_can_afford = 0;
				int fake_gold = soft_data->gold;
				int gold_to_spend = 0;
				for(int i = 0; i < how_many_to_buy; i += 1) {
					int cost = data.cost + (get_upgrade_level(upgrade_i) + i) * data.extra_cost_per_level;
					cost_all += cost;
					if(can_afford(fake_gold, cost)) {
						fake_gold -= cost;
						gold_to_spend += cost;
						how_many_can_afford += 1;
					}
				}
				s_len_str str = zero;
				s_button_data optional = zero;
				if(how_many_can_afford == 0) {
					optional.disabled = true;
				}
				if(is_upgrade_maxed(upgrade_i)) {
					str = format_text("%.*s (MAX)", expand_str(data.name));
					optional.disabled = true;
				}
				else {
					str = format_text("%.*s (%i)", expand_str(data.name), cost_all);
				}
				optional.tooltip = get_upgrade_description(upgrade_i);
				optional.mute_click_sound = true;
				optional.font_size = 28;
				int key = (int)SDLK_1 + upgrade_i;
				if(do_button_ex(str, pos, button_size, false, optional) || (!optional.disabled && is_key_pressed(key, true))) {
					add_gold(-gold_to_spend);
					apply_upgrade(upgrade_i, how_many_can_afford);
					play_sound(e_sound_upgrade, {.speed = get_rand_sound_speed(1.1f, &game->rng)});
					game->purchased_at_least_one_upgrade = true;
				}
			}

			{
				b8 should_do_upgrade_tutorial = false;
				for_enum(upgrade_i, e_upgrade) {
					s_upgrade_data data = g_upgrade_data[upgrade_i];
					if(!game->purchased_at_least_one_upgrade && can_afford(soft_data->gold, data.cost)) {
						should_do_upgrade_tutorial = true;
					}
				}

				if(should_do_upgrade_tutorial) {
					s_v4 color = hsv_to_rgb(game->render_time * 360, 1, 1);
					draw_text(S("Buy upgrades! ->"), wxy(0.62f, 0.28f), sin_range(32, 40, game->render_time * 8), color, true, &game->font, zero);
				}

				if(!completed_attack_tutorial()) {
					s_len_str str = format_text("Press %sleft click$. or ", c_key_color_str);
					float font_size = sin_range(32, 40, game->render_time * 8);
					s_v2 pos = draw_text(str, gxy(0.42f, 0.8f), font_size, make_color(1), true, &game->font, zero);
					draw_keycap(scancode_to_char(SDL_SCANCODE_S), pos, v2(font_size));
					pos.x += font_size;
					draw_text(S(" to attack"), pos, font_size, make_color(1), false, &game->font, zero);
					draw_text(S("(There is a cooldown if you don't hit any enemies)"), gxy(0.5f, 0.85f), 32, make_color(0.8f), true, &game->font, zero);
				}
				else if(game->num_times_we_dashed < 2) {
					s_len_str str = format_text("Press %sright click$. or ", c_key_color_str);
					float font_size = sin_range(32, 40, game->render_time * 8);
					s_v2 pos = draw_text(str, gxy(0.44f, 0.8f), font_size, make_color(1), true, &game->font, zero);
					draw_keycap(scancode_to_char(SDL_SCANCODE_A), pos, v2(font_size));
					pos.x += font_size;
					draw_text(S(" to dash"), pos, font_size, make_color(1), false, &game->font, zero);
					draw_text(S("(Attacks do triple damage while dashing)"), gxy(0.5f, 0.85f), 32, make_color(0.8f), true, &game->font, zero);
				}
			}

			{
				float passed = game->render_time - soft_data->gold_change_timestamp;
				float font_size = ease_out_elastic_advanced(passed, 0, 0.5f, 64, 48);
				float t = ease_linear_advanced(passed, 0, 1, 1, 0);
				s_v4 color = lerp_color(make_color(0.8f, 0.8f, 0), make_color(1, 1, 0.3f), t);
				draw_text(format_text("Gold: %i", soft_data->gold), wxy(0.87f, 0.04f), font_size, color, true, &game->font, zero);
			}
			{
				float passed = game->render_time - soft_data->life_change_timestamp;
				float font_size = ease_out_elastic_advanced(passed, 0, 0.5f, 64, 48);
				float t = ease_linear_advanced(passed, 0, 1, 1, 0);
				s_v4 color = lerp_color(hex_to_rgb(0xDF20AF), hex_to_rgb(0xDF204F), t);
				draw_text(format_text("Lives: %i", get_max_lives() - soft_data->lives_lost), wxy(0.87f, 0.12f), font_size, color, true, &game->font, zero);
			}
			{
				s_time_format data = update_count_to_time_format(game->hard_data.update_count);
				s_len_str text = format_text("%02i:%02i.%03i", data.minutes, data.seconds, data.milliseconds);
				draw_text(text, wxy(0.87f, 0.2f), 48, make_color(1), true, &game->font, zero);
			}

			if(game->hover_over_upgrade_pauses_game && are_we_hovering_over_ui(g_mouse) && !game->do_hard_reset) {
				game->speed = 0;
				draw_text(S("Paused"), gxy(0.5f, 0.1f), sin_range(64, 80, game->render_time * 8), make_color(1), true, &game->font, zero);
			}
			else {
				game->speed = 1;
			}

			{
				s_render_flush_data data = make_render_flush_data(zero, zero);
				data.projection = ortho;
				data.blend_mode = e_blend_mode_normal;
				data.depth_mode = e_depth_mode_read_and_write;
				render_flush(data, true);
			}

			if(game->tooltip.count > 0) {
				float font_size = 32;
				s_v2 size = get_text_size(game->tooltip, &game->font, font_size);
				s_v2 rect_pos = zero;
				rect_pos -= v2(8);
				size += v2(16);
				rect_pos = topleft_to_bottomleft_mouse(rect_pos, size, g_mouse);
				rect_pos = prevent_offscreen(rect_pos, size);
				s_v2 text_pos = rect_pos + v2(8);
				draw_rect_topleft(rect_pos, size, make_color(0.0f, 0.95f));
				draw_text(game->tooltip, text_pos, font_size, make_color(1), false, &game->font, zero);
				{
					s_render_flush_data data = make_render_flush_data(zero, zero);
					data.projection = ortho;
					data.blend_mode = e_blend_mode_normal;
					data.depth_mode = e_depth_mode_no_read_no_write;
					render_flush(data, true);
				}
			}
			game->tooltip = zero;
		}

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		cheat menu start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		#if defined(m_debug)
		if(game->cheat_menu_enabled) {
			s_rect rect = {
				10, 10, 350, c_world_size.y
			};
			s_v2 size = v2(320, 48);
			s_container container = make_down_center_x_container(rect, size, 10);

			if(do_button_ex(S("+100000 gold"), container_get_pos_and_advance(&container), size, false, zero)) {
				add_gold(100000);
			}
			if(do_button_ex(S("Spawn boss"), container_get_pos_and_advance(&container), size, false, zero)) {
				spawn_enemy(e_enemy_boss);
			}
			if(do_button_ex(S("Win"), container_get_pos_and_advance(&container), size, false, zero)) {
				soft_data->frame_data.boss_defeated = true;
			}
			if(do_button_ex(S("Lose"), container_get_pos_and_advance(&container), size, false, zero)) {
				soft_data->frame_data.lives_to_lose = 99999;
			}
			if(do_button_ex(S("Spawn basic enemy"), container_get_pos_and_advance(&container), size, false, zero)) {
				spawn_enemy(e_enemy_basic);
			}

			{
				s_render_flush_data data = make_render_flush_data(zero, zero);
				data.projection = ortho;
				data.blend_mode = e_blend_mode_normal;
				data.depth_mode = e_depth_mode_no_read_no_write;
				render_flush(data, true);
			}
		}
		#endif
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		cheat menu end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	}

	if(do_defeat) {

		draw_rect_topleft(v2(0), c_world_size, make_color(0, 0.75f));
		draw_text(S("Defeat"), wxy(0.5f, 0.4f) + rand_v2_11(&game->rng) * 8, 128, make_color(1, 0.2f, 0.2f), true, &game->font, zero);
		draw_text(S("Press R to try again"), wxy(0.5f, 0.55f), sin_range(48, 64, game->render_time * 8), make_color(1), true, &game->font, zero);

		{
			s_render_flush_data data = make_render_flush_data(zero, zero);
			data.projection = ortho;
			data.blend_mode = e_blend_mode_normal;
			data.depth_mode = e_depth_mode_no_read_no_write;
			render_flush(data, true);
		}
	}

	s_state_transition transition = get_state_transition(&game->state0, game->render_time);
	if(transition.active) {
		{
			float alpha = 0;
			if(transition.time_data.percent <= 0.5f) {
				alpha = transition.time_data.percent * 2;
			}
			else {
				alpha = transition.time_data.inv_percent * 2;
			}
			s_instance_data data = zero;
			data.model = fullscreen_m4();
			data.color = make_color(0.0f, 0, 0, alpha);
			add_to_render_group(data, e_shader_flat, e_texture_white, e_mesh_quad);
		}

		{
			s_render_flush_data data = make_render_flush_data(zero, zero);
			data.projection = ortho;
			data.blend_mode = e_blend_mode_normal;
			data.depth_mode = e_depth_mode_no_read_no_write;
			render_flush(data, true);
		}
	}

	SDL_GL_SwapWindow(g_platform_data->window);

	game->render_time += delta;
}

func f64 get_seconds()
{
	u64 now =	SDL_GetPerformanceCounter();
	return (now - g_platform_data->start_cycles) / (f64)g_platform_data->cycle_frequency;
}

func void on_gl_error(const char* expr, char* file, int line, int error)
{
	#define m_gl_errors \
	X(GL_INVALID_ENUM, "GL_INVALID_ENUM") \
	X(GL_INVALID_VALUE, "GL_INVALID_VALUE") \
	X(GL_INVALID_OPERATION, "GL_INVALID_OPERATION") \
	X(GL_STACK_OVERFLOW, "GL_STACK_OVERFLOW") \
	X(GL_STACK_UNDERFLOW, "GL_STACK_UNDERFLOW") \
	X(GL_OUT_OF_MEMORY, "GL_STACK_OUT_OF_MEMORY") \
	X(GL_INVALID_FRAMEBUFFER_OPERATION, "GL_STACK_INVALID_FRAME_BUFFER_OPERATION")

	const char* error_str;
	#define X(a, b) case a: { error_str = b; } break;
	switch(error) {
		m_gl_errors
		default: {
			error_str = "unknown error";
		} break;
	}
	#undef X
	#undef m_gl_errors

	printf("GL ERROR: %s - %i (%s)\n", expr, error, error_str);
	printf("  %s(%i)\n", file, line);
	printf("--------\n");

	#if defined(_WIN32)
	__debugbreak();
	#else
	__builtin_trap();
	#endif
}

func void draw_rect(s_v2 pos, s_v2 size, s_v4 color)
{
	s_instance_data data = zero;
	data.model = m4_translate(v3(pos, 0));
	data.model = m4_multiply(data.model, m4_scale(v3(size, 1)));
	data.color = color;

	add_to_render_group(data, e_shader_flat, e_texture_white, e_mesh_quad);
}

func void draw_rect_topleft(s_v2 pos, s_v2 size, s_v4 color)
{
	pos += size * 0.5f;
	draw_rect(pos, size, color);
}

func void draw_texture_screen(s_v2 pos, s_v2 size, s_v4 color, e_texture texture_id, e_shader shader_id, s_v2 uv_min, s_v2 uv_max, s_draw_data draw_data)
{
	s_instance_data data = zero;
	data.model = m4_translate(v3(pos, draw_data.z));
	data.model = m4_multiply(data.model, m4_scale(v3(size, 1)));
	data.color = color;
	data.uv_min = uv_min;
	data.uv_max = uv_max;

	add_to_render_group(data, shader_id, texture_id, e_mesh_quad);
}

func void draw_mesh(e_mesh mesh_id, s_m4 model, s_v4 color, e_shader shader_id)
{
	s_instance_data data = zero;
	data.model = model;
	data.color = color;
	add_to_render_group(data, shader_id, e_texture_white, mesh_id);
}

func void draw_mesh(e_mesh mesh_id, s_v3 pos, s_v3 size, s_v4 color, e_shader shader_id)
{
	s_m4 model = m4_translate(pos);
	model = m4_multiply(model, m4_scale(size));
	draw_mesh(mesh_id, model, color, shader_id);
}

func void bind_framebuffer(u32 fbo)
{
	if(game->curr_fbo != fbo) {
		gl(glBindFramebuffer(GL_FRAMEBUFFER, fbo));
		game->curr_fbo = fbo;
	}
}

func void clear_framebuffer_color(u32 fbo, s_v4 color)
{
	bind_framebuffer(fbo);
	glClearColor(color.x, color.y, color.z, color.w);
	glClear(GL_COLOR_BUFFER_BIT);
}

func void clear_framebuffer_depth(u32 fbo)
{
	bind_framebuffer(fbo);
	set_depth_mode(e_depth_mode_read_and_write);
	glClear(GL_DEPTH_BUFFER_BIT);
}

func void render_flush(s_render_flush_data data, b8 reset_render_count)
{
	bind_framebuffer(data.fbo.id);

	if(data.fbo.id == 0) {
		s_rect letterbox = do_letterbox(v2(g_platform_data->window_size), c_world_size);
		glViewport((int)letterbox.x, (int)letterbox.y, (int)letterbox.w, (int)letterbox.h);
	}
	else {
		glViewport(0, 0, data.fbo.size.x, data.fbo.size.y);
	}

	set_cull_mode(data.cull_mode);
	set_depth_mode(data.depth_mode);
	set_blend_mode(data.blend_mode);

	{
		gl(glBindBuffer(GL_UNIFORM_BUFFER, game->ubo));
		s_uniform_data uniform_data = zero;
		uniform_data.view = data.view;
		uniform_data.projection = data.projection;
		uniform_data.light_view = data.light_view;
		uniform_data.light_projection = data.light_projection;
		uniform_data.render_time = game->render_time;
		uniform_data.cam_pos = data.cam_pos;
		uniform_data.mouse = g_mouse;
		uniform_data.player_pos = data.player_pos;
		uniform_data.window_size.x = (float)g_platform_data->window_size.x;
		uniform_data.window_size.y = (float)g_platform_data->window_size.y;
		gl(glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(s_uniform_data), &uniform_data));
	}

	foreach_val(group_i, group, game->render_group_arr) {
		s_mesh* mesh = &game->mesh_arr[group.mesh_id];
		int* instance_count = &game->render_instance_count[group.shader_id][group.texture_id][group.mesh_id];
		assert(*instance_count > 0);
		void* instance_data = game->render_instance_arr[group.shader_id][group.texture_id][group.mesh_id];

		gl(glUseProgram(game->shader_arr[group.shader_id].id));

		int in_texture_loc = glGetUniformLocation(game->shader_arr[group.shader_id].id, "in_texture");
		int noise_loc = glGetUniformLocation(game->shader_arr[group.shader_id].id, "noise");
		if(in_texture_loc >= 0) {
			glUniform1i(in_texture_loc, 0);
			glActiveTexture(GL_TEXTURE0);
			gl(glBindTexture(GL_TEXTURE_2D, game->texture_arr[group.texture_id].id));
		}
		if(noise_loc >= 0) {
			glUniform1i(noise_loc, 2);
			glActiveTexture(GL_TEXTURE2);
			gl(glBindTexture(GL_TEXTURE_2D, game->texture_arr[e_texture_noise].id));
		}

		gl(glBindVertexArray(mesh->vao));
		gl(glBindBuffer(GL_ARRAY_BUFFER, mesh->instance_vbo.id));
		gl(glBufferSubData(GL_ARRAY_BUFFER, 0, group.element_size * *instance_count, instance_data));
		gl(glDrawArraysInstanced(GL_TRIANGLES, 0, mesh->vertex_count, *instance_count));
		if(reset_render_count) {
			game->render_group_arr.remove_and_swap(group_i);
			game->render_group_index_arr[group.shader_id][group.texture_id][group.mesh_id] = -1;
			group_i -= 1;
			*instance_count = 0;
		}
	}
}

template <typename t>
func void add_to_render_group(t data, e_shader shader_id, e_texture texture_id, e_mesh mesh_id)
{
	s_render_group render_group = zero;
	render_group.shader_id = shader_id;
	render_group.texture_id = texture_id;
	render_group.mesh_id = mesh_id;
	render_group.element_size = sizeof(t);

	s_mesh* mesh = &game->mesh_arr[render_group.mesh_id];

	int* render_group_index = &game->render_group_index_arr[render_group.shader_id][render_group.texture_id][render_group.mesh_id];
	if(*render_group_index < 0) {
		game->render_group_arr.add(render_group);
		*render_group_index = game->render_group_arr.count - 1;
	}
	int* count = &game->render_instance_count[render_group.shader_id][render_group.texture_id][render_group.mesh_id];
	int* max_elements = &game->render_instance_max_elements[render_group.shader_id][render_group.texture_id][render_group.mesh_id];
	t* ptr = (t*)game->render_instance_arr[render_group.shader_id][render_group.texture_id][render_group.mesh_id];
	b8 expand = *max_elements <= *count;
	b8 get_new_ptr = *count <= 0 || expand;
	int new_max_elements = *max_elements;
	if(expand) {
		if(new_max_elements <= 0) {
			new_max_elements = 64;
		}
		else {
			new_max_elements *= 2;
		}
		if(new_max_elements > mesh->instance_vbo.max_elements) {
			gl(glBindBuffer(GL_ARRAY_BUFFER, mesh->instance_vbo.id));
			gl(glBufferData(GL_ARRAY_BUFFER, sizeof(t) * new_max_elements, null, GL_DYNAMIC_DRAW));
			mesh->instance_vbo.max_elements = new_max_elements;
		}
	}
	if(get_new_ptr) {
		t* temp = (t*)arena_alloc(&game->render_frame_arena, sizeof(t) * new_max_elements);
		if(*count > 0) {
			memcpy(temp, ptr, *count * sizeof(t));
		}
		game->render_instance_arr[render_group.shader_id][render_group.texture_id][render_group.mesh_id] = (void*)temp;
		ptr = temp;
	}
	*max_elements = new_max_elements;
	*(ptr + *count) = data;
	*count += 1;
}

func s_shader load_shader_from_file(char* file, s_linear_arena* arena)
{
	b8 delete_shaders[2] = {true, true};
	char* src = (char*)read_file(file, arena);
	assert(src);

	u32 shader_arr[] = {glCreateShader(GL_VERTEX_SHADER), glCreateShader(GL_FRAGMENT_SHADER)};

	#if defined(__EMSCRIPTEN__)
	const char* header = "#version 300 es\nprecision highp float;\n";
	#else
	const char* header = "#version 330 core\n";
	#endif

	char* shared_src = (char*)try_really_hard_to_read_file("src/shader_shared.h", arena);
	assert(shared_src);

	for(int i = 0; i < 2; i += 1) {
		const char* src_arr[] = {header, "", "", shared_src, src};
		if(i == 0) {
			src_arr[1] = "#define m_vertex 1\n";
			src_arr[2] = "#define shared_var out\n";
		}
		else {
			src_arr[1] = "#define m_fragment 1\n";
			src_arr[2] = "#define shared_var in\n";
		}
		gl(glShaderSource(shader_arr[i], array_count(src_arr), (const GLchar * const *)src_arr, null));
		gl(glCompileShader(shader_arr[i]));

		int compile_success;
		char info_log[1024];
		gl(glGetShaderiv(shader_arr[i], GL_COMPILE_STATUS, &compile_success));

		if(!compile_success) {
			gl(glGetShaderInfoLog(shader_arr[i], sizeof(info_log), null, info_log));
			printf("Failed to compile shader: %s\n%s", file, info_log);
			delete_shaders[i] = false;
		}
	}

	b8 detach_shaders = delete_shaders[0] && delete_shaders[1];

	u32 program = 0;
	if(delete_shaders[0] && delete_shaders[1]) {
		program = gl(glCreateProgram());
		gl(glAttachShader(program, shader_arr[0]));
		gl(glAttachShader(program, shader_arr[1]));
		gl(glLinkProgram(program));

		int linked = 0;
		gl(glGetProgramiv(program, GL_LINK_STATUS, &linked));
		if(!linked) {
			char info_log[1024] = zero;
			gl(glGetProgramInfoLog(program, sizeof(info_log), null, info_log));
			printf("FAILED TO LINK: %s\n", info_log);
		}
	}

	if(detach_shaders) {
		gl(glDetachShader(program, shader_arr[0]));
		gl(glDetachShader(program, shader_arr[1]));
	}

	if(delete_shaders[0]) {
		gl(glDeleteShader(shader_arr[0]));
	}
	if(delete_shaders[1]) {
		gl(glDeleteShader(shader_arr[1]));
	}

	s_shader result = zero;
	result.id = program;
	return result;
}

func b8 do_button(s_len_str text, s_v2 pos, b8 centered)
{
	s_v2 size = v2(256, 48);
	b8 result = do_button_ex(text, pos, size, centered, zero);
	return result;
}

func b8 do_button_ex(s_len_str text, s_v2 pos, s_v2 size, b8 centered, s_button_data optional)
{
	b8 result = false;
	if(!centered) {
		pos += size * 0.5f;
	}

	b8 do_tooltip = false;
	b8 hovered = mouse_vs_rect_center(g_mouse, pos, size);
	s_v4 color = make_color(0.25f);
	s_v4 text_color = make_color(1);
	if(hovered) {
		do_tooltip = true;
	}
	if(optional.disabled) {
		hovered = false;
		color = make_color(0.15f);
		text_color = make_color(0.7f);
	}
	if(hovered) {
		size += v2(8);
		if(!centered) {
			pos -= v2(4) * 0.5f;
		}
		color = make_color(0.5f);
		if(g_left_click) {
			result = true;
			if(!optional.mute_click_sound) {
				play_sound(e_sound_click, zero);
			}
		}
	}

	{
		s_instance_data data = zero;
		data.model = m4_translate(v3(pos, 0));
		data.model = m4_multiply(data.model, m4_scale(v3(size, 1)));
		data.color = color;
		add_to_render_group(data, e_shader_button, e_texture_white, e_mesh_quad);
	}

	draw_text(text, pos, optional.font_size, text_color, true, &game->font, {.z = 1});

	if(do_tooltip && optional.tooltip.count > 0) {
		game->tooltip = optional.tooltip;
	}

	return result;
}

func b8 do_bool_button(s_len_str text, s_v2 pos, b8 centered, b8* out)
{
	s_v2 size = v2(256, 48);
	b8 result = do_bool_button_ex(text, pos, size, centered, out);
	return result;
}

func b8 do_bool_button_ex(s_len_str text, s_v2 pos, s_v2 size, b8 centered, b8* out)
{
	assert(out);
	b8 result = false;
	if(do_button_ex(text, pos, size, centered, zero)) {
		result = true;
		*out = !(*out);
	}
	return result;
}

func b8 is_key_pressed(int key, b8 consume)
{
	b8 result = false;
	if(key < c_max_keys) {
		result = game->input_arr[key].half_transition_count > 1 || (game->input_arr[key].half_transition_count > 0 && game->input_arr[key].is_down);
	}
	if(result && consume) {
		game->input_arr[key].half_transition_count = 0;
		game->input_arr[key].is_down = false;
	}
	return result;
}

func b8 is_key_down(int key)
{
	b8 result = false;
	if(key < c_max_keys) {
		result = game->input_arr[key].half_transition_count > 1 || game->input_arr[key].is_down;
	}
	return result;
}

template <int n>
func void cstr_into_builder(s_str_builder<n>* builder, char* str)
{
	assert(str);

	int len = (int)strlen(str);
	assert(len <= n);
	memcpy(builder->str, str, len);
	builder->count = len;
}

template <int n>
func s_len_str builder_to_len_str(s_str_builder<n>* builder)
{
	s_len_str result = zero;
	result.str = builder->str;
	result.count = builder->count;
	return result;
}

template <int n>
func b8 handle_string_input(s_input_str<n>* str, float time)
{
	b8 result = false;
	if(!str->cursor.valid) {
		str->cursor = maybe(0);
	}
	foreach_val(c_i, c, game->char_events) {
		if(is_alpha_numeric(c) || c == '_') {
			if(!is_builder_full(&str->str)) {
				builder_insert_char(&str->str, str->cursor.value, c);
				str->cursor.value += 1;
				str->last_edit_time = time;
				str->last_action_time = str->last_edit_time;
			}
		}
	}

	foreach_val(event_i, event, game->key_events) {
		if(!event.went_down) { continue; }
		if(event.key == SDLK_RETURN) {
			result = true;
			str->last_action_time = time;
		}
		else if(event.key == SDLK_ESCAPE) {
			str->cursor.value = 0;
			str->str.count = 0;
			str->str.str[0] = 0;
			str->last_edit_time = time;
			str->last_action_time = str->last_edit_time;
		}
		else if(event.key == SDLK_BACKSPACE) {
			if(str->cursor.value > 0) {
				str->cursor.value -= 1;
				builder_remove_char_at(&str->str, str->cursor.value);
				str->last_edit_time = time;
				str->last_action_time = str->last_edit_time;
			}
		}
	}
	return result;
}

func void handle_key_event(int key, b8 is_down, b8 is_repeat)
{
	if(is_down) {
		game->any_key_pressed = true;
	}
	if(key < c_max_keys) {
		if(!is_repeat) {
			game->any_key_pressed = true;
		}

		{
			s_key_event key_event = {};
			key_event.went_down = is_down;
			key_event.key = key;
			// key_event.modifiers |= e_input_modifier_ctrl * is_key_down(&g_platform_data.input, c_key_left_ctrl);
			game->key_events.add(key_event);
		}
	}
}

func void do_leaderboard()
{
	b8 escape = is_key_pressed(SDLK_ESCAPE, true);
	if(do_button(S("Back"), wxy(0.87f, 0.92f), true) || escape) {
		s_maybe<int> prev = get_previous_non_temporary_state(&game->state0);
		if(prev.valid && prev.value == e_game_state0_pause) {
			pop_state_transition(&game->state0, game->render_time, c_transition_time);
		}
		else {
			add_state_transition(&game->state0, e_game_state0_main_menu, game->render_time, c_transition_time);
			clear_state(&game->state0);
		}
	}

	{
		if(!game->leaderboard_received) {
			draw_text(S("Getting leaderboard..."), c_world_center, 48, make_color(0.66f), true, &game->font, zero);
		}
		else if(game->leaderboard_arr.count <= 0) {
			draw_text(S("No scores yet :("), c_world_center, 48, make_color(0.66f), true, &game->font, zero);
		}

		constexpr int c_max_visible_entries = 10;
		s_v2 pos = c_world_center * v2(1.0f, 0.7f);
		for(int entry_i = 0; entry_i < at_most(c_max_visible_entries + 1, game->leaderboard_arr.count); entry_i += 1) {
			s_leaderboard_entry entry = game->leaderboard_arr[entry_i];
			s_time_format data = update_count_to_time_format(entry.time);
			s_v4 color = make_color(0.8f);
			int rank_number = entry_i + 1;
			if(entry_i == c_max_visible_entries || builder_equals(&game->leaderboard_public_uid, &entry.internal_name)) {
				color = hex_to_rgb(0xD3A861);
				rank_number = entry.rank;
			}
			char* name = entry.internal_name.str;
			if(entry.nice_name.count > 0) {
				name = entry.nice_name.str;
			}
			draw_text(format_text("%i %s", rank_number, name), v2(c_world_size.x * 0.1f, pos.y - 24), 32, color, false, &game->font, zero);
			s_len_str text = format_text("%02i:%02i.%i", data.minutes, data.seconds, data.milliseconds);
			draw_text(text, v2(c_world_size.x * 0.5f, pos.y - 24), 32, color, false, &game->font, zero);
			pos.y += 48;
		}
	}
}

func s_v2 get_rect_normal_of_closest_edge(s_v2 p, s_v2 center, s_v2 size)
{
	s_v2 edge_arr[] = {
		v2(center.x - size.x * 0.5f, center.y),
		v2(center.x + size.x * 0.5f, center.y),
		v2(center.x, center.y - size.y * 0.5f),
		v2(center.x, center.y + size.y * 0.5f),
	};
	s_v2 normal_arr[] = {
		v2(-1, 0),
		v2(1, 0),
		v2(0, -1),
		v2(0, 1),
	};
	float closest_dist[2] = {9999999, 9999999};
	int closest_index[2] = {0, 0};

	for(int i = 0; i < 4; i += 1) {
		float dist;
		if(i <= 1) {
			dist = fabsf(p.x - edge_arr[i].x);
		}
		else {
			dist = fabsf(p.y - edge_arr[i].y);
		}
		if(dist < closest_dist[0]) {
			closest_dist[0] = dist;
			closest_index[0] = i;
		}
		else if(dist < closest_dist[1]) {
			closest_dist[1] = dist;
			closest_index[1] = i;
		}
	}
	s_v2 result = normal_arr[closest_index[0]];

	// @Note(tkap, 19/04/2025): Probably breaks for very thin rectangles
	if(fabsf(closest_dist[0] - closest_dist[1]) <= 0.01f) {
		result += normal_arr[closest_index[1]];
		result = v2_normalized(result);
	}
	return result;
}

func b8 is_valid_2d_index(s_v2i index, int x_count, int y_count)
{
	b8 result = index.x >= 0 && index.x < x_count && index.y >= 0 && index.y < y_count;
	return result;
}

func b8 check_action(float curr_time, float timestamp, float grace)
{
	float passed = curr_time - timestamp;
	b8 result = passed <= grace && timestamp > 0;
	return result;
}

func void draw_atlas(s_v2 pos, s_v2 size, s_v2i index, s_v4 color)
{
	draw_atlas_ex(pos, size, index, color, 0, zero);
}

func void draw_atlas_ex(s_v2 pos, s_v2 size, s_v2i index, s_v4 color, float rotation, s_draw_data draw_data)
{
	s_instance_data data = zero;
	data.model = m4_translate(v3(pos, draw_data.z));
	if(rotation != 0) {
		data.model *= m4_rotate(rotation, v3(0, 0, 1));
	}
	data.model *= m4_scale(v3(size, 1));
	data.color = color;
	int x = index.x * c_atlas_sprite_size + c_atlas_padding;
	data.uv_min.x = x / (float)c_atlas_size_v.x;
	data.uv_max.x = data.uv_min.x + (c_atlas_sprite_size - c_atlas_padding) / (float)c_atlas_size_v.x;
	int y = index.y * c_atlas_sprite_size + c_atlas_padding;
	data.uv_min.y = y / (float)(c_atlas_size_v.y);
	data.uv_max.y = data.uv_min.y + (c_atlas_sprite_size - c_atlas_padding) / (float)c_atlas_size_v.y;
	data.mix_weight = draw_data.mix_weight;
	data.mix_color = draw_data.mix_color;

	if(draw_data.flip_x) {
		swap(&data.uv_min.x, &data.uv_max.x);
	}

	add_to_render_group(data, e_shader_flat_remove_black, e_texture_atlas, e_mesh_quad);
}

func void draw_atlas_topleft(s_v2 pos, s_v2 size, s_v2i index, s_v4 color)
{
	pos += size * 0.5f;
	draw_atlas(pos, size, index, color);
}

func void draw_circle(s_v2 pos, float radius, s_v4 color)
{
	s_instance_data data = zero;
	data.model = m4_translate(v3(pos, 0));
	data.model = m4_multiply(data.model, m4_scale(v3(radius * 2, radius * 2, 1)));
	data.color = color;

	add_to_render_group(data, e_shader_circle, e_texture_white, e_mesh_quad);
}

func void draw_light(s_v2 pos, float radius, s_v4 color, float smoothness)
{
	s_instance_data data = zero;
	data.model = m4_translate(v3(pos, 0));
	data.model = m4_multiply(data.model, m4_scale(v3(radius * 2, radius * 2, 1)));
	data.color = color;
	data.mix_weight = smoothness;

	add_to_render_group(data, e_shader_light, e_texture_white, e_mesh_quad);
}

func s_v2 get_upgrade_offset(float interp_dt)
{
	float t = update_time_to_render_time(game->update_time, interp_dt);
	s_v2 result = v2(0.0f, sinf(t * 3.14f) * 3);
	return result;
}

func void do_screen_shake(float intensity)
{
	s_soft_game_data* soft_data = &game->soft_data;
	soft_data->start_screen_shake_timestamp = game->render_time;
	soft_data->shake_intensity = intensity;
}

func void add_timed_msg(s_len_str str, s_v2 pos)
{
	s_timed_msg msg = zero;
	msg.pos = pos;
	msg.spawn_timestamp = game->update_time;
	str_into_builder(&msg.builder, str);
	game->soft_data.timed_msg_arr.add_if_not_full(msg);
}

func void draw_background(s_v2 player_pos, s_m4 ortho)
{
	if(game->hide_background) { return; }

	{
		s_instance_data data = zero;
		data.model = m4_translate(v3(c_world_center, 0));
		data.model = m4_multiply(data.model, m4_scale(v3(c_world_size, 1)));
		data.color = make_color(1);
		add_to_render_group(data, e_shader_background, e_texture_white, e_mesh_quad);
	}

	{
		s_render_flush_data data = make_render_flush_data(zero, v3(player_pos, 0.0f));
		data.projection = ortho;
		data.blend_mode = e_blend_mode_normal;
		data.depth_mode = e_depth_mode_read_no_write;
		render_flush(data, true);
	}
}


func void teleport_entity(s_entity* entity, s_v2 pos)
{
	entity->pos = pos;
	entity->prev_pos = pos;
}

func float get_player_damage()
{
	float result = 0;
	result += 10;
	result *= 1.0f + get_upgrade_boost(e_upgrade_damage) / 100.0f;
	if(timer_is_active(game->soft_data.dash_timer, game->update_time, c_dash_duration)) {
		result *= 3;
	}
	return result;
}

func float get_player_attack_range()
{
	float result = 0;
	result += c_player_attack_range;
	result *= 1.0f + get_upgrade_boost(e_upgrade_range) / 100.0f;
	return result;
}


func float get_player_speed()
{
	float result = 0;
	result += 1.5f;
	result *= 1.0f + get_upgrade_boost(e_upgrade_speed) / 100.0f;
	return result;
}

func float get_enemy_max_health(e_enemy type)
{
	float result = c_enemy_health;
	result *= g_enemy_type_data[type].health_multi;
	return result;
}

func b8 damage_enemy(s_entity* enemy, float damage)
{
	b8 dead = false;
	float max_health = get_enemy_max_health(enemy->enemy_type);
	enemy->damage_taken += damage;
	enemy->hit_timestamp = game->update_time;
	if(enemy->damage_taken >= max_health) {
		dead = true;
	}

	{
		s_entity fct = zero;
		fct.spawn_timestamp = game->render_time;
		builder_add(&fct.builder, "%.0f", damage);
		fct.duration = 1.5f;
		fct.pos = enemy->pos;
		fct.vel.x = randf32_11(&game->rng) * 50;
		entity_manager_add(&game->soft_data.entity_arr, e_entity_fct, fct);
	}

	return dead;
}

func void make_dying_enemy(s_entity enemy)
{
	s_entity dying_enemy = enemy;
	entity_manager_add(&game->soft_data.entity_arr, e_entity_dying_enemy, dying_enemy);
}

func s_particle_emitter_a make_emitter_a()
{
	s_particle_emitter_a result = zero;
	result.particle_duration = 1;
	result.radius = 16;
	result.speed = 128;
	{
		s_particle_color color = zero;
		color.color = make_color(1);
		result.color_arr.add(color);
	}
	{
		s_particle_color color = zero;
		color.color = make_color(0.0f);
		color.percent = 1;
		result.color_arr.add(color);
	}
	return result;
}

func s_particle_emitter_b make_emitter_b()
{
	s_particle_emitter_b result = zero;
	result.particles_per_second = 1;
	result.particle_count = 1;
	return result;
}

func int add_emitter(s_entity emitter)
{
	s_soft_game_data* soft_data = &game->soft_data;
	emitter.emitter_b.creation_timestamp = game->render_time;
	emitter.emitter_b.last_emit_timestamp = game->render_time - 1.0f / emitter.emitter_b.particles_per_second;
	int index = entity_manager_add(&soft_data->entity_arr, e_entity_emitter, emitter);
	return index;
}

func s_v2 gxy(float x, float y)
{
	s_v2 result = {x * c_game_area.x, y * c_game_area.y};
	return result;
}

func s_v2 gxy(float x)
{
	s_v2 result = gxy(x, x);
	return result;
}


func s_container make_center_x_container(s_rect rect, s_v2 element_size, float padding, int element_count)
{
	assert(element_count > 0);

	s_container result = zero;
	float space_used = (element_size.x * element_count) + (padding * (element_count - 1));
	float space_left = rect.size.x - space_used;
	result.advance.x = element_size.x + padding;
	result.curr_pos.x = rect.pos.x + space_left * 0.5f;
	result.curr_pos.y = rect.pos.y + rect.size.y * 0.5f - element_size.y * 0.5f;
	return result;
}

func s_container make_forward_container(s_rect rect, s_v2 element_size, float padding)
{
	s_container result = zero;
	result.advance.x = element_size.x + padding;
	result.curr_pos.x = rect.pos.x + padding;
	result.curr_pos.y = rect.pos.y + rect.size.y * 0.5f - element_size.y * 0.5f;
	return result;
}

func s_container make_down_center_x_container(s_rect rect, s_v2 element_size, float padding)
{
	s_container result = zero;
	result.advance.y = element_size.y + padding;
	result.curr_pos.x = rect.pos.x + rect.size.x * 0.5f - element_size.x * 0.5f;
	result.curr_pos.y = rect.pos.y + padding;
	return result;
}

func s_container make_forward_align_right_container(s_rect rect, s_v2 element_size, float padding, float edge_padding, int element_count)
{
	s_container result = zero;
	float space_used = (element_size.x * element_count) + (padding * (element_count - 1));
	result.advance.x = element_size.x + padding;
	result.curr_pos.x = rect.size.x - space_used - edge_padding;
	result.curr_pos.y = rect.pos.y + rect.size.y * 0.5f - element_size.y * 0.5f;
	return result;
}

func s_v2 container_get_pos_and_advance(s_container* c)
{
	s_v2 result = c->curr_pos;
	c->curr_pos += c->advance;
	return result;
}

func b8 can_afford(int curr_gold, int cost)
{
	b8 result = curr_gold >= cost;
	return result;
}

func void apply_upgrade(e_upgrade id, int count)
{
	game->soft_data.upgrade_count[id] += count;
}

func int get_upgrade_level(e_upgrade id)
{
	int result = game->soft_data.upgrade_count[id];
	return result;
}

func float get_upgrade_boost(e_upgrade id)
{
	int level = get_upgrade_level(id);
	if(level > 0 && id == e_upgrade_lightning_bolt) {
		level -= 1;
	}
	else if(level > 0 && id == e_upgrade_auto_attack) {
		level -= 1;
	}
	float result = level * g_upgrade_data[id].stat_boost;
	return result;
}

func s_v2i get_enemy_atlas_index(e_enemy type)
{
	s_v2i result = zero;
	result = g_enemy_type_data[type].atlas_index;
	return result;
}

func s_v2 get_enemy_size(e_enemy type)
{
	s_v2 result = g_enemy_type_data[type].size;
	return result;
}

func float get_player_knockback()
{
	float result = c_knockback;
	result *= 1.0f + get_upgrade_boost(e_upgrade_knockback) / 100.0f;
	return result;
}

func void add_gold(int gold)
{
	game->soft_data.gold += gold;
	game->soft_data.gold_change_timestamp = game->render_time;
}

func void lose_lives(int how_many)
{
	game->soft_data.frame_data.lives_to_lose += how_many;
}

func void spawn_enemy(e_enemy type)
{
	s_entity enemy = zero;
	enemy.enemy_type = type;
	enemy.spawn_timestamp = game->update_time;
	s_v2 pos = gxy(0.5f) + v2_from_angle(randf_range(&game->rng, 0, c_tau)) * c_game_area.x * 0.6f;
	teleport_entity(&enemy, pos);
	entity_manager_add(&game->soft_data.entity_arr, e_entity_enemy, enemy);
}

func float get_dash_cooldown()
{
	float result = c_dash_cooldown;
	result *= 1.0f - get_upgrade_boost(e_upgrade_dash_cooldown) / 100.0f;
	return result;
}

func b8 is_upgrade_maxed(e_upgrade id)
{
	int max_upgrades = g_upgrade_data[id].max_upgrades;
	b8 result = false;
	if(max_upgrades > 0 && game->soft_data.upgrade_count[id] >= max_upgrades) {
		result = true;
	}
	return result;
}

func int get_max_lives()
{
	int result = c_max_lives;
	result += (int)get_upgrade_boost(e_upgrade_max_lives);
	return result;
}

func int get_hits_per_attack()
{
	int result = 1;
	result += (int)get_upgrade_boost(e_upgrade_more_hits_per_attack);
	return result;
}

func float get_lightning_bolt_cooldown()
{
	float frequency = 1.0f / c_lightning_bolt_cooldown;
	frequency *= 1.0f + get_upgrade_boost(e_upgrade_lightning_bolt) / 100.0f;
	float result = 1.0f / frequency;
	return result;
}

func float get_lightning_bolt_frequency()
{
	float cd = get_lightning_bolt_cooldown();
	float result = 1.0f / cd;
	return result;
}

func s_v2 topleft_to_bottomleft_mouse(s_v2 pos, s_v2 size, s_v2 mouse)
{
	s_v2 result = pos;
	result += mouse;
	result.x -= pos.x;
	result.y -= pos.y;
	result.y -= size.y;
	return result;
}

func s_v2 prevent_offscreen(s_v2 pos, s_v2 size)
{
	s_v2 result = pos;
	float* ptr[4] = {
		&result.x, &result.x, &result.y, &result.y
	};
	// left, right, up, down
	float diff[4] = {
		pos.x - 0,
		c_world_size.x - (pos.x + size.x),
		pos.y - 0,
		c_world_size.y - (pos.y + size.y),
	};
	for(int i = 0; i < 4; i += 1) {
		if(diff[i] < 0) {
			float x = i % 2 == 0 ? -1.0f : 1.0f;
			*ptr[i] += diff[i] * x;
		}
	}
	return result;
}

func s_len_str get_upgrade_description(e_upgrade id)
{
	int level = game->soft_data.upgrade_count[id];
	s_upgrade_data data = g_upgrade_data[id];

	s_str_builder<512> builder;
	builder.count = 0;
	switch(id) {
		xcase e_upgrade_damage: {
			builder_add(&builder, "+%.0f%% damage\n\n", data.stat_boost);
			// builder_add(&builder, "Press %sleft click$. or %s%c$. to attack\n\n", c_key_color_str, c_key_color_str, to_upper_case(scancode_to_char(SDL_SCANCODE_S)));
			builder_add(&builder, "Current: %.0f", get_player_damage());
		};
		xcase e_upgrade_speed: {
			builder_add(&builder, "+%.0f%% movement speed\n\n", data.stat_boost);
			builder_add(&builder, "Current: %.2f", get_player_speed());
		};
		xcase e_upgrade_range: {
			builder_add(&builder, "+%.0f%% attack range\n\n", data.stat_boost);
			builder_add(&builder, "Current: %.0f", get_player_attack_range());
		};
		xcase e_upgrade_knockback: {
			builder_add(&builder, "+%.0f%% knockback\n\n", data.stat_boost);
			builder_add(&builder, "Current: %.0f", get_player_knockback());
		};
		xcase e_upgrade_dash_cooldown: {
			builder_add(&builder, "-%.0f%% dash cooldown\n", data.stat_boost);
			// builder_add(&builder, "Press %sright click$. or %s%c$. to dash\n", c_key_color_str, c_key_color_str, to_upper_case(scancode_to_char(SDL_SCANCODE_A)));
			builder_add(&builder, "Attacks do triple damage while dashing\n\n", c_key_color_str, c_key_color_str, to_upper_case(scancode_to_char(SDL_SCANCODE_A)));
			builder_add(&builder, "Current: %.2f second cooldown", get_dash_cooldown());
		};
		xcase e_upgrade_max_lives: {
			builder_add(&builder, "+%.0f max lives\n\n", data.stat_boost);
			builder_add(&builder, "Current: %i", get_max_lives());
		};
		xcase e_upgrade_more_hits_per_attack: {
			builder_add(&builder, "+%.0f enemy hit per attack\n\n", data.stat_boost);
			builder_add(&builder, "Current: %i", get_hits_per_attack());
		};
		xcase e_upgrade_lightning_bolt: {
			if(level == 0) {
				builder_add(&builder, "A lightning bolt strikes a nearby\nenemy every %.2f seconds", get_lightning_bolt_cooldown());
			}
			else {
				builder_add(&builder, "Lightning bolts strike with %.0f%% increased frequency\n\n", data.stat_boost);
				builder_add(&builder, "Current: %.2f hits per second", get_lightning_bolt_frequency());
			}
		};
		xcase e_upgrade_auto_attack: {
			if(level == 0) {
				builder_add(&builder, "Attacks happen automatically every %.2f second%s\nNo more clicking!", get_auto_attack_cooldown(), handle_plural(get_auto_attack_cooldown()));
			}
			else {
				builder_add(&builder, "Auto attacks have %.0f%% increased frequency\n\n", data.stat_boost);
				float frequency = get_auto_attack_frequency();
				builder_add(&builder, "Current: %.2f hit%s per second", frequency, handle_plural(frequency));
			}
		};
		break; invalid_default_case;
	}
	int key = (int)SDLK_1 + id;
	builder_add(&builder, "\n\nHotkey [%s%c$.]", c_key_color_str, '1' + key - SDLK_1);

	s_len_str temp = builder_to_len_str(&builder);
	s_len_str result = format_text("%.*s", expand_str(temp));
	return result;
}

func b8 are_we_hovering_over_ui(s_v2 mouse)
{
	b8 result = mouse.x > c_game_area.x;
	return result;
}

func b8 do_we_have_auto_attack()
{
	b8 result = get_upgrade_level(e_upgrade_auto_attack) > 0;
	return result;
}

func float get_auto_attack_cooldown()
{
	float frequency = 1.0f / c_auto_attack_cooldown;
	frequency *= 1.0f + get_upgrade_boost(e_upgrade_auto_attack) / 100.0f;
	float result = 1.0f / frequency;
	return result;
}

func float get_auto_attack_frequency()
{
	float result = 1.0f / get_auto_attack_cooldown();
	return result;
}

func float get_attack_or_auto_attack_cooldown()
{
	float result = 0;
	if(do_we_have_auto_attack()) {
		result = get_auto_attack_cooldown();
	}
	else {
		result = c_attack_cooldown_on_miss;
	}
	return result;
}

func char* handle_plural(float x)
{
	char* result = "s";
	if(fabsf(x) == 1.0f) {
		result = "";
	}
	return result;
}

func s_entity make_enemy_death_particles(s_v2 pos)
{
	s_entity emitter = zero;

	emitter.emitter_a = make_emitter_a();
	emitter.emitter_a.pos = v3(pos, 0.0f);
	emitter.emitter_a.dir.x = 0.5f;
	emitter.emitter_a.dir.y = -1.0f;
	emitter.emitter_a.dir_rand = zero;
	emitter.emitter_a.dir_rand.x = 1;
	emitter.emitter_a.particle_duration *= 0.5f;
	emitter.emitter_a.radius *= 0.5f;
	emitter.emitter_a.color_arr[0].color = make_color(0.1f);

	emitter.emitter_a.particle_duration_rand = 1.0f;
	emitter.emitter_a.radius_rand = 1.0f;
	emitter.emitter_a.speed_rand = 1.0f;

	emitter.emitter_b = make_emitter_b();
	emitter.emitter_b.spawn_type = e_emitter_spawn_type_circle;
	emitter.emitter_b.spawn_data.x = 10;
	emitter.emitter_b.particle_count = 200;

	return emitter;
}

func s_entity make_enemy_hit_particles(s_v2 pos)
{
	s_entity emitter = zero;

	emitter.emitter_a = make_emitter_a();
	emitter.emitter_a.dir = v3(1, 1, 0);
	emitter.emitter_a.dir_rand = v3(1, 1, 0);
	emitter.emitter_a.pos = v3(pos, 0.0f);
	emitter.emitter_a.particle_duration *= 0.33f;
	emitter.emitter_a.radius *= 0.5f;
	emitter.emitter_a.color_arr[0].color = make_color(0.5f, 0.1f, 0.1f);

	emitter.emitter_a.particle_duration_rand = 1.0f;
	emitter.emitter_a.radius_rand = 1.0f;
	emitter.emitter_a.speed_rand = 1.0f;

	emitter.emitter_b = make_emitter_b();
	emitter.emitter_b.particle_count = 200;

	return emitter;
}

func s_entity make_circle_particles()
{
	s_entity emitter = zero;

	emitter.emitter_a = make_emitter_a();
	emitter.emitter_a.dir = v3(0.1f, -0.5f, 0);
	emitter.emitter_a.dir_rand = v3(1, 0, 0);
	emitter.emitter_a.pos = v3(gxy(0.5f), 0.0f);
	emitter.emitter_a.particle_duration *= 2;
	emitter.emitter_a.radius *= 0.5f;
	// emitter.emitter_a.color_arr[0].color = make_color(0.5f, 0.1f, 0.1f);
	emitter.emitter_a.color_arr.count = 4;
	float strength = 0.2f;
	emitter.emitter_a.color_arr[0].color = make_color(strength);
	emitter.emitter_a.color_arr[1].color = make_color(strength, strength, 0.0f);
	emitter.emitter_a.color_arr[2].color = make_color(strength, 0.0f, 0.0f);
	emitter.emitter_a.color_arr[3].color = make_color(0.0f);
	emitter.emitter_a.color_arr[0].percent = 0.0f;
	emitter.emitter_a.color_arr[1].percent = 0.2f;
	emitter.emitter_a.color_arr[2].percent = 0.6f;
	emitter.emitter_a.color_arr[3].percent = 1.0f;

	emitter.emitter_a.particle_duration_rand = 0.5f;
	emitter.emitter_a.radius_rand = 0.5f;
	emitter.emitter_a.speed_rand = 1.0f;

	emitter.emitter_b = make_emitter_b();
	emitter.emitter_b.particle_count = 10;
	emitter.emitter_b.particles_per_second = 100;
	emitter.emitter_b.duration = -1;
	emitter.emitter_b.spawn_type = e_emitter_spawn_type_circle_outline;
	emitter.emitter_b.spawn_data.x = c_circle_radius * 0.5f;

	return emitter;
}

template <typename t, typename F>
func void radix_sort_32(t* source, u32 count, F get_radix, s_linear_arena* arena)
{
	if(count == 0) { return; }

	t* destination = (t*)arena_alloc(arena, sizeof(t) * count);

	for(u32 index_i = 0; index_i < 32; index_i += 8) {
		u32 offsets[256] = zero;

		// @Note(tkap, 02/07/2024): Count how many of each value between 0-255 we have
		// [0, 1, 1, 1, 2, 2] == [1, 3, 2]
		for(u32 i = 0; i < count; i += 1) {
			u32 val = get_radix(source[i]);
			u32 mask = 0xff << index_i;
			u32 radix_val = (val & mask) >> index_i;
			offsets[radix_val] += 1;
		}

		// @Note(tkap, 02/07/2024): Turn the offsets array from an array of counts into an array of offsets
		u32 total = 0;
		for(u32 i = 0; i < 256; i += 1) {
			u32 temp_count = offsets[i];
			offsets[i] = total;
			total += temp_count;
		}

		// @Note(tkap, 02/07/2024): Modify the new list
		for(u32 i = 0; i < count; i += 1) {
			u32 val = get_radix(source[i]);
			u32 mask = 0xff << index_i;
			u32 radix_val = (val & mask) >> index_i;

			u32 destination_index = offsets[radix_val];
			offsets[radix_val] += 1;
			destination[destination_index] = source[i];
		}

		t* swap_temp = destination;
		destination = source;
		source = swap_temp;
	}
}

func u32 get_radix_from_enemy_index(int index)
{
	assert(game->soft_data.entity_arr.active[index]);
	float dist = v2_distance_squared(game->soft_data.entity_arr.data[index].pos, gxy(0.5f));
	u32 result = float_to_radix(dist);
	return result;
}

func s_entity make_lose_lives_particles()
{
	s_entity emitter = zero;

	emitter.emitter_a = make_emitter_a();
	emitter.emitter_a.pos = v3(gxy(0.5f), 0.0f);
	emitter.emitter_a.dir = v3(0.4f, -1.0f, 0.0);
	emitter.emitter_a.dir_rand = v3(1, 0, 0);
	emitter.emitter_a.particle_duration *= 2.0f;
	emitter.emitter_a.radius *= 0.5f;
	emitter.emitter_a.gravity = 10;
	emitter.emitter_a.speed = 1000;
	emitter.emitter_a.color_arr[0].color = make_color(1.0f, 0.1f, 0.1f);

	emitter.emitter_a.particle_duration_rand = 1.0f;
	emitter.emitter_a.radius_rand = 1.0f;
	emitter.emitter_a.speed_rand = 0.5f;

	emitter.emitter_b = make_emitter_b();
	emitter.emitter_b.spawn_type = e_emitter_spawn_type_circle;
	emitter.emitter_b.spawn_data.x = 32;
	emitter.emitter_b.particle_count = 200;

	return emitter;
}

func void draw_keycap(char c, s_v2 pos, s_v2 size)
{
	pos += size * 0.5f;
	draw_atlas_ex(pos, size, v2i(124, 42), make_color(1), 0, zero);
	s_len_str str = format_text("%c", to_upper_case(c));
	pos.y -= size.x * 0.05f;
	draw_text(str, pos, size.x, c_key_color, true, &game->font, {.z = 1});
}

func b8 completed_attack_tutorial()
{
	b8 result = game->num_times_we_attacked_an_enemy >= 3;
	return result;
}

func void add_multiplicative_light(s_v2 pos, float radius, s_v4 color, float smoothness)
{
	s_light light = zero;
	light.pos = pos;
	light.radius = radius;
	light.color = color;
	light.smoothness = smoothness;
	if(!game->multiplicative_light_arr.is_full()) {
		game->multiplicative_light_arr.add(light);
	}
}

func void add_additive_light(s_v2 pos, float radius, s_v4 color, float smoothness)
{
	s_light light = zero;
	light.pos = pos;
	light.radius = radius;
	light.color = color;
	light.smoothness = smoothness;
	if(!game->additive_light_arr.is_full()) {
		game->additive_light_arr.add(light);
	}
}
