
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
global b8 g_click;

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
	game->speed = 1;

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
		platform_data->sound_arr[i] = platform_data->load_sound_from_file(c_sound_data_arr[i].path, c_sound_data_arr[i].volume);
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

	u8* keyboard_state = (u8*)SDL_GetKeyboardState(null);

	if(game->input.handled) {
		game->input = zero;
	}
	// game->input.left = game->input.left || keyboard_state[SDL_SCANCODE_A];
	// game->input.right = game->input.right || keyboard_state[SDL_SCANCODE_D];


	for(int i = 0; i < c_max_keys; i += 1) {
		game->input_arr[i].half_transition_count = 0;
	}

	g_click = false;
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
				if(key == SDLK_LSHIFT) {
					key = c_left_shift;
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
					else if(key == SDLK_f && event.key.repeat == 0) {
						soft_data->tried_to_attack_timestamp = game->update_time;
					}
					else if(key == SDLK_g && event.key.repeat == 0) {
						soft_data->want_to_dash_timestamp = game->update_time;
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
					g_click = true;
				}
				if(event.type == SDL_MOUSEBUTTONDOWN && event.button.button == 3) {
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

	memset(&soft_data->frame_data, 0, sizeof(soft_data->frame_data));

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

				{
					s_entity enemy = zero;
					enemy.enemy_type = chosen_enemy_type;
					enemy.spawn_timestamp = game->update_time;
					s_v2 pos = gxy(0.5f) + v2_from_angle(randf_range(&game->rng, 0, c_tau)) * c_game_area.x * 0.6f;
					teleport_entity(&enemy, pos);
					entity_manager_add(entity_arr, e_entity_enemy, enemy);
				}
			}
		}
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		spawn end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		update enemies start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		for(int i = c_first_index[e_entity_enemy]; i < c_last_index_plus_one[e_entity_enemy]; i += 1) {
			if(!entity_arr->active[i]) { continue; }
			s_entity* enemy = &entity_arr->data[i];
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
				float speed = 25 * g_enemy_type_data[enemy->enemy_type].speed_multi;
				s_time_data time_data = get_time_data(game->update_time, enemy->spawn_timestamp, 5.0f);
				if(time_data.percent <= 1) {
					speed += time_data.inv_percent * 100;
				}
				enemy->pos += dir * speed * delta;
				float dist = v2_distance(enemy->pos, gxy(0.5f));
				if(dist <= 10) {
					lose_lives(1);
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
			float time_since_dash_ended = game->update_time - (soft_data->did_dash_timestamp + c_dash_duration);
			b8 is_dash_on_cooldown = time_since_dash_ended < c_dash_cooldown && soft_data->did_dash_timestamp > 0;
			if(check_action(game->update_time, soft_data->want_to_dash_timestamp, 0.1f) && !is_dash_on_cooldown) {
				play_sound(e_sound_dash, {.volume = 0.1f});
				soft_data->did_dash_timestamp = game->update_time;
				soft_data->want_to_dash_timestamp = 0;
			}

			float dash_speed = 1;
			{
				s_time_data time_data = get_time_data(game->update_time, soft_data->did_dash_timestamp, c_dash_duration);
				if(time_data.percent >= 0.0f && time_data.percent <= 1.0f) {
					dash_speed = time_data.inv_percent * 10;
				}
			}
			player->timer += delta * get_player_speed() * dash_speed;

			// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		try to hit start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
			b8 highlighted_an_enemy = false;
			float attack_range = get_player_attack_range();
			for(int i = c_first_index[e_entity_enemy]; i < c_last_index_plus_one[e_entity_enemy]; i += 1) {
				if(!entity_arr->active[i]) { continue; }
				s_entity* enemy = &entity_arr->data[i];

				float dist = v2_distance(enemy->pos, player->pos);
				if(dist <= attack_range + get_enemy_size(enemy->enemy_type).y * 0.5f) {
					if(!highlighted_an_enemy) {
						highlighted_an_enemy = true;
						enemy->highlight = maybe(make_color(1, 0.6f, 0.6f));
					}
					if(check_action(game->update_time, soft_data->tried_to_attack_timestamp, 0.1f)) {
						play_sound(e_sound_key, zero);
						float knockback_to_add = 750 * get_enemy_knockback_resistance_taking_into_account_upgrades(enemy->enemy_type);
						if(enemy->knockback.valid) {
							enemy->knockback.value += knockback_to_add;
						}
						else {
							enemy->knockback = maybe(knockback_to_add);
						}

						soft_data->tried_to_attack_timestamp = 0;
						player->did_attack_enemy_timestamp = game->update_time;
						player->attacked_enemy_pos = enemy->pos;
						b8 dead = damage_enemy(enemy, get_player_damage());
						if(dead) {
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
		int curr_lives = c_max_lives - game->soft_data.lives_lost;
		if(curr_lives <= 0) {
			add_state(&game->hard_data.state1, e_game_state1_defeat);
		}
	}
	game->input.handled = true;
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

	s_m4 light_projection = make_orthographic(-50, 50, -50, 50, -50, 50);
	s_v3 sun_pos = v3(0, -10, 10);
	s_v3 sun_dir = v3_normalized(v3(1, 1, -1));
	s_m4 ortho = make_orthographic(0, c_world_size.x, c_world_size.y, 0, -1, 1);
	s_m4 perspective = make_perspective(60.0f, c_world_size.x / c_world_size.y, 0.01f, 100.0f);

	game->speed = 1;

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

			draw_text(c_game_name, wxy(0.5f, 0.2f), 128, make_color(1), true, &game->font);
			draw_text(S("www.twitch.tv/Tkap1"), wxy(0.5f, 0.3f), 32, make_color(0.6f), true, &game->font);

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

			draw_text(c_game_name, wxy(0.5f, 0.2f), 128, make_color(1), true, &game->font);
			draw_text(S("www.twitch.tv/Tkap1"), wxy(0.5f, 0.3f), 32, make_color(0.6f), true, &game->font);

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
				draw_text(text, c_world_center * v2(1.0f, 0.2f), 64, make_color(1), true, &game->font);

				draw_text(S("Press R to restart..."), c_world_center * v2(1.0f, 0.4f), sin_range(48, 60, game->render_time * 8.0f), make_color(0.66f), true, &game->font);
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

			draw_text(S("Victory!"), c_world_size * v2(0.5f, 0.1f), font_size, make_color(1), true, &game->font);
			draw_text(S("Enter your name"), c_world_size * v2(0.5f, 0.2f), font_size, make_color(1), true, &game->font);
			if(state->error_str.count > 0) {
				draw_text(builder_to_len_str(&state->error_str), c_world_size * v2(0.5f, 0.3f), font_size, hex_to_rgb(0xD77870), true, &game->font);
			}

			if(state->name.str.count > 0) {
				draw_text(builder_to_len_str(&state->name.str), pos, font_size, make_color(1), true, &game->font);
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
			constexpr int tile_size = 32;
			int tiles_right = floorfi(c_game_area.x / tile_size);
			int tiles_down = ceilfi(c_game_area.y / tile_size);
			for(int y = 0; y < tiles_down; y += 1) {
				for(int x = 0; x < tiles_right; x += 1) {
					s_v2 pos = v2(x * tile_size, y * tile_size) + v2(tile_size * 0.5f);
					draw_atlas(pos, v2(tile_size), v2i(2, 6), make_color(1));
				}
			}

			s_render_flush_data data = make_render_flush_data(zero, zero);
			data.projection = ortho;
			data.blend_mode = e_blend_mode_normal;
			data.depth_mode = e_depth_mode_no_read_no_write;
			render_flush(data, true);
		}
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		tiles end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		draw_circle(gxy(0.5f, 0.5f), c_circle_radius, make_color(0.2f));

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		draw enemies start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		{
			for(int i = c_first_index[e_entity_enemy]; i < c_last_index_plus_one[e_entity_enemy]; i += 1) {
				if(!entity_arr->active[i]) { continue; }
				s_entity* enemy = &entity_arr->data[i];
				s_v2 enemy_pos = lerp_v2(enemy->prev_pos, enemy->pos, interp_dt);
				s_v4 color = make_color(1);
				if(enemy->highlight.valid) {
					color = enemy->highlight.value;
				}
				draw_atlas(enemy_pos, get_enemy_size(enemy->enemy_type), get_enemy_atlas_index(enemy->enemy_type), color);
			}
		}
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		draw enemies end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		draw dying enemies start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		{
			for(int i = c_first_index[e_entity_dying_enemy]; i < c_last_index_plus_one[e_entity_dying_enemy]; i += 1) {
				if(!entity_arr->active[i]) { continue; }
				s_entity* enemy = &entity_arr->data[i];
				s_v2 enemy_pos = lerp_v2(enemy->prev_pos, enemy->pos, interp_dt);
				s_v4 color = enemy->highlight.value;
				draw_atlas(enemy_pos, get_enemy_size(enemy->enemy_type), get_enemy_atlas_index(enemy->enemy_type), color);
			}
		}
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		draw dying enemies end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv		draw player start		vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
		{
			s_v2 player_pos = lerp_v2(player->prev_pos, player->pos, interp_dt);
			draw_rect(player_pos, c_player_size_v, make_color(0, 1, 0));
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

					if(player->did_attack_enemy_timestamp > 0) {
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

					draw_rect(pos, size, make_color(0, 1, 0));
				}
			}
		}
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^		draw player end		^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

		update_particles(delta);

		{
			s_render_flush_data data = make_render_flush_data(zero, zero);
			data.projection = ortho;
			data.blend_mode = e_blend_mode_additive;
			data.depth_mode = e_depth_mode_no_read_no_write;
			render_flush(data, true);
		}

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
				draw_text(str, fct->pos, 32, color, true, &game->font);
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

		if(do_game_ui) {

			s_rect rect = {
				c_game_area.x, 176.0f, c_world_size.x - c_game_area.x, c_world_size.y
			};
			s_v2 size = v2(320, 48);
			s_container container = make_down_center_x_container(rect, size, 10);

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
				s_v2 pos = container_get_pos_and_advance(&container);
				int cost = data.cost + get_upgrade_level(upgrade_i) * data.extra_cost_per_level;
				s_len_str str = format_text("%.*s (%i)", expand_str(data.name), cost);
				s_button_data optional = zero;
				if(!can_afford(soft_data->gold, cost)) {
					optional.disabled = true;
				}
				if(do_button_ex(str, pos, size, false, optional)) {
					add_gold(-cost);
					apply_upgrade(upgrade_i, 1);
				}
			}
			{
				float passed = game->render_time - soft_data->gold_change_timestamp;
				float font_size = ease_out_elastic_advanced(passed, 0, 0.5f, 64, 48);
				float t = ease_linear_advanced(passed, 0, 1, 1, 0);
				s_v4 color = lerp_color(make_color(0.8f, 0.8f, 0), make_color(1, 1, 0.3f), t);
				draw_text(format_text("Gold: %i", soft_data->gold), wxy(0.87f, 0.04f), font_size, color, true, &game->font);
			}
			{
				float passed = game->render_time - soft_data->life_change_timestamp;
				float font_size = ease_out_elastic_advanced(passed, 0, 0.5f, 64, 48);
				float t = ease_linear_advanced(passed, 0, 1, 1, 0);
				s_v4 color = lerp_color(hex_to_rgb(0xDF20AF), hex_to_rgb(0xDF204F), t);
				draw_text(format_text("Lives: %i", c_max_lives - soft_data->lives_lost), wxy(0.87f, 0.12f), font_size, color, true, &game->font);
			}
			{
				s_time_format data = update_count_to_time_format(game->hard_data.update_count);
				s_len_str text = format_text("%02i:%02i.%03i", data.minutes, data.seconds, data.milliseconds);
				draw_text(text, wxy(0.87f, 0.2f), 48, make_color(1), true, &game->font);
			}

			{
				s_render_flush_data data = make_render_flush_data(zero, zero);
				data.projection = ortho;
				data.blend_mode = e_blend_mode_normal;
				data.depth_mode = e_depth_mode_no_read_no_write;
				render_flush(data, true);
			}
		}

		#if defined(m_debug)
		if(game->cheat_menu_enabled) {
			s_rect rect = {
				10, 10, 350, c_world_size.y
			};
			s_v2 size = v2(320, 48);
			s_container container = make_down_center_x_container(rect, size, 10);

			if(do_button_ex(S("+1000 gold"), container_get_pos_and_advance(&container), size, false, zero)) {
				add_gold(1000);
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
	}

	if(do_defeat) {

		draw_rect_topleft(v2(0), c_world_size, make_color(0, 0.75f));
		draw_text(S("Defeat"), wxy(0.5f, 0.4f) + rand_v2_11(&game->rng) * 8, 128, make_color(1, 0.2f, 0.2f), true, &game->font);
		draw_text(S("Press R to try again"), wxy(0.5f, 0.55f), sin_range(48, 64, game->render_time * 8), make_color(1), true, &game->font);

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

func void draw_texture_screen(s_v2 pos, s_v2 size, s_v4 color, e_texture texture_id, e_shader shader_id, s_v2 uv_min, s_v2 uv_max)
{
	s_instance_data data = zero;
	data.model = m4_translate(v3(pos, 0));
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

	b8 hovered = mouse_vs_rect_center(g_mouse, pos, size);
	s_v4 color = make_color(0.25f);
	s_v4 text_color = make_color(1);
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
		if(g_click) {
			result = true;
			play_sound(e_sound_click, zero);
		}
	}

	{
		s_instance_data data = zero;
		data.model = m4_translate(v3(pos, 0));
		data.model = m4_multiply(data.model, m4_scale(v3(size, 1)));
		data.color = color;
		add_to_render_group(data, e_shader_button, e_texture_white, e_mesh_quad);
	}

	draw_text(text, pos, 32.0f, text_color, true, &game->font);

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
			draw_text(S("Getting leaderboard..."), c_world_center, 48, make_color(0.66f), true, &game->font);
		}
		else if(game->leaderboard_arr.count <= 0) {
			draw_text(S("No scores yet :("), c_world_center, 48, make_color(0.66f), true, &game->font);
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
			draw_text(format_text("%i %s", rank_number, name), v2(c_world_size.x * 0.1f, pos.y - 24), 32, color, false, &game->font);
			s_len_str text = format_text("%02i:%02i.%i", data.minutes, data.seconds, data.milliseconds);
			draw_text(text, v2(c_world_size.x * 0.5f, pos.y - 24), 32, color, false, &game->font);
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
	draw_atlas_ex(pos, size, index, color, 0);
}

func void draw_atlas_ex(s_v2 pos, s_v2 size, s_v2i index, s_v4 color, float rotation)
{
	s_instance_data data = zero;
	data.model = m4_translate(v3(pos, 0));
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

func void draw_light(s_v2 pos, float radius, s_v4 color)
{
	s_instance_data data = zero;
	data.model = m4_translate(v3(pos, 0));
	data.model = m4_multiply(data.model, m4_scale(v3(radius * 2, radius * 2, 1)));
	data.color = color;

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
	float result = 0;
	result += g_enemy_type_data[type].max_health;
	return result;
}

func b8 damage_enemy(s_entity* enemy, float damage)
{
	b8 dead = false;
	float max_health = get_enemy_max_health(enemy->enemy_type);
	enemy->damage_taken += damage;
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
	float result = get_upgrade_level(id) * g_upgrade_data[id].stat_boost;
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

func float get_enemy_knockback_resistance_taking_into_account_upgrades(e_enemy type)
{
	float result = 1.0f - g_enemy_type_data[type].knockback_resistance + get_upgrade_boost(e_upgrade_knockback);
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