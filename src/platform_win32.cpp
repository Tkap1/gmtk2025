
#pragma warning(push, 0)
#define SDL_MAIN_HANDLED
#include "SDL2/SDL.h"
#include "SDL_mixer.h"
#pragma warning(pop)

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#undef near
#undef far

#include "tklib.h"
#include "platform_win32.h"
#include "shared.h"
global s_platform_data g_platform_data;

typedef void (*t_game_func)(s_platform_data*);

#if defined(m_debug)
t_game_func do_game = null;
#endif

int main()
{

	g_platform_data.memory = (u8*)calloc(1, 512 * 1024 * 1024);

	SDL_SetMainReady();
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	Mix_OpenAudioDevice(44100, MIX_DEFAULT_FORMAT, 2, 512, NULL, 0);
	Mix_Volume(-1, floorfi(MIX_MAX_VOLUME * 0.1f));

	b8 load_dll = true;
	b8 first_load = true;

	#if defined(m_debug)
	CreateThread(null, 0, watch_for_file_changes, null, 0, null);
	HMODULE dll = null;
	t_game_func init = null;
	t_game_func init_after_recompile = null;
	#endif

	while(!g_platform_data.quit) {

		#if defined(m_debug)
		while(g_platform_data.hot_read_index[0] < g_platform_data.hot_write_index) {
			char* path = g_platform_data.hot_file_arr[g_platform_data.hot_read_index[0] % c_max_hot_files];
			assert(strlen(path) > 0);
			if(strcmp(path, "main.dll") == 0) {
				load_dll = true;
			}
			g_platform_data.hot_read_index[0] += 1;
		}

		if(load_dll) {
			load_dll = false;
			if(dll) {
				FreeLibrary(dll);
			}

			for(int i = 0; i < 100; i++) {
				if(CopyFile("main.dll", "temp_main.dll", false)) { break; }
				Sleep(10);
			}

			dll = LoadLibrary("temp_main.dll");
			assert(dll);
			printf("Reloaded DLL\n");

			init = (t_game_func)GetProcAddress(dll, "init");
			init_after_recompile = (t_game_func)GetProcAddress(dll, "init_after_recompile");
			do_game = (t_game_func)GetProcAddress(dll, "do_game");
			assert(init);
			assert(init_after_recompile);
			assert(do_game);

			if(first_load) {
				first_load = false;
				init(&g_platform_data);
			}
			init_after_recompile(&g_platform_data);

		}

		#endif

		do_one_frame();
	}

	return 0;
}

func void do_one_frame()
{
	do_game(&g_platform_data);
}

#if defined(m_debug)
func DWORD WINAPI watch_for_file_changes(void* arg)
{
	(void)arg;
	HANDLE dir = CreateFile(".", GENERIC_READ, FILE_SHARE_READ, null, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, null);
	while(true) {
		FILE_NOTIFY_INFORMATION buffer[32] = zero;
		DWORD bytes_returned = 0;
		ReadDirectoryChangesW(dir, buffer, sizeof(buffer), true, FILE_NOTIFY_CHANGE_LAST_WRITE, &bytes_returned, null, null);
		FILE_NOTIFY_INFORMATION* curr = &buffer[0];
		while(true) {
			WCHAR* name = curr->FileName;
			char* builder = g_platform_data.hot_file_arr[g_platform_data.hot_write_index % c_max_hot_files];
			int len = curr->FileNameLength / 2;
			for(int i = 0; i < len; i += 1) {
				builder[i] = (char)name[i];
				builder[i + 1] = '\0';
			}
			g_platform_data.hot_write_index += 1;
			if(curr->NextEntryOffset) {
				curr = (FILE_NOTIFY_INFORMATION*)((u8*)curr + curr->NextEntryOffset);
			}
			else { break; }
		}
	}

	return 0;
}
#endif