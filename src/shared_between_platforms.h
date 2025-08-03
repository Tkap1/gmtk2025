
#include "generated/generated_shared_between_platforms.h"

func s_loaded_sound load_sound_from_file(char* path)
{
	s_loaded_sound result = zero;
	SDL_LoadWAV(path, &result.spec, &result.data, &result.size_in_bytes);
	assert(result.spec.freq == 44100);
	assert(result.spec.format == 32784);
	assert(result.spec.channels == 1);
	return result;
}

func void init_common()
{
	g_platform_data.load_sound_from_file = load_sound_from_file;
	g_platform_data.play_sound = play_sound;

	{
		SDL_AudioSpec desired_spec;
    desired_spec.freq = 44100;
    desired_spec.format = AUDIO_F32SYS;
    desired_spec.channels = 1;
    desired_spec.samples = 512;
    desired_spec.callback = my_audio_callback;

		SDL_AudioSpec obtained_spec = zero;

		SDL_AudioDeviceID device = SDL_OpenAudioDevice(null, 0, &desired_spec, &obtained_spec, 0);
		// SDL_AudioDeviceID device = SDL_OpenAudioDevice(null, 0, &desired_spec, &obtained_spec, SDL_AUDIO_ALLOW_ANY_CHANGE);
		#if defined(m_debug)
		printf("Audio device %i\n", device);
		if(device == 0) {
			printf("%s\n", SDL_GetError());
		}
		#endif
		SDL_PauseAudioDevice(device, 0);
	}
}

func void my_audio_callback(void* userdata, u8* stream, int len) {
	(void)userdata;
	assert(len % sizeof(float) == 0);
	int sample_count = len / sizeof(float);
	float* ptr = (float*)stream;
	for(int i = 0; i < sample_count; i += 1) {
		float value = 0;
		foreach_ptr(sound_i, sound, g_platform_data.active_sound_arr) {
			s_loaded_sound loaded_sound = g_platform_data.sound_arr[sound->loaded_sound_id];
			int sound_sample_count = loaded_sound.size_in_bytes / sizeof(s16);
			s16* sound_sample_arr = (s16*)loaded_sound.data;
			float percent = floorfi(sound->index) / (float)sound_sample_count;
			float fade_volume = 1;
			if(sound->data.fade.valid) {
				if(percent >= sound->data.fade.value.percent[0]) {
					float t = ilerp(sound->data.fade.value.percent[0], sound->data.fade.value.percent[1], percent);
					fade_volume = lerp(sound->data.fade.value.volume[0], sound->data.fade.value.volume[1], t);
				}
			}

			{
				float left_index_f = sound->index;
				int left_index_i = floorfi(left_index_f);
				float left_sample0 = sound_sample_arr[left_index_i] / (float)c_max_s16;
				float left_sample1 = 0;
				if(left_index_i + 1 >= sound_sample_count) {
					left_sample1 = left_sample0;
				}
				else {
					left_sample1 = sound_sample_arr[left_index_i + 1] / (float)c_max_s16;
				}
				value += lerp(left_sample0, left_sample1, fract(sound->index)) * sound->data.volume * fade_volume;
			}

			sound->index += sound->data.speed;
			if(floorfi(sound->index) >= sound_sample_count) {
				if(sound->data.loop) {
					sound->index -= sound_sample_count;
				}
				else {
					g_platform_data.active_sound_arr.remove_and_swap(sound_i);
					sound_i -= 1;
				}
			}
		}
		ptr[0] = value;
		ptr += 1;
	}
}

func void play_sound(e_sound sound_id, s_play_sound_data data)
{
	data.volume *= c_sound_data_arr[sound_id].volume;
	if(!g_platform_data.active_sound_arr.is_full()) {
		s_active_sound active_sound = zero;
		active_sound.loaded_sound_id = sound_id;
		active_sound.data = data;
		g_platform_data.active_sound_arr.add(active_sound);
	}
}
