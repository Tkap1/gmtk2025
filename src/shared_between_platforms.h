
#include "generated/generated_shared_between_platforms.h"

func Mix_Chunk* load_sound_from_file(char* path, u8 volume)
{
	Mix_Chunk* chunk = Mix_LoadWAV(path);
	assert(chunk);
	chunk->volume = volume;
	return chunk;
}

func void init_common()
{
	g_platform_data.load_sound_from_file = load_sound_from_file;
	g_platform_data.play_sound = play_sound;

	{
		SDL_AudioSpec desired_spec;
    desired_spec.freq = 44100;
    desired_spec.format = AUDIO_F32SYS;
    desired_spec.channels = 2;
    desired_spec.samples = 512;
    desired_spec.callback = my_audio_callback;

		SDL_AudioSpec obtained_spec = zero;

		SDL_AudioDeviceID device = SDL_OpenAudioDevice(null, 0, &desired_spec, &obtained_spec, 0);
		SDL_PauseAudioDevice(device, 0);
	}
}

func void my_audio_callback(void* userdata, u8* stream, int len) {
	(void)userdata;
	assert(len % sizeof(float) == 0);
	int sample_count = len / sizeof(float) / 2;
	float* ptr = (float*)stream;
	for(int i = 0; i < sample_count; i += 1) {
		float value_left = 0;
		float value_right = 0;
		foreach_ptr(sound_i, sound, g_platform_data.active_sound_arr) {
			int sound_sample_count = sound->chunk->alen / sizeof(s16);
			s16* sound_sample_arr = (s16*)sound->chunk->abuf;
			float percent = floorfi(sound->index) * 2 / (float)sound_sample_count;
			float fade_volume = 1;
			if(sound->data.fade.valid) {
				if(percent >= sound->data.fade.value.percent[0]) {
					float t = ilerp(sound->data.fade.value.percent[0], sound->data.fade.value.percent[1], percent);
					fade_volume = lerp(sound->data.fade.value.volume[0], sound->data.fade.value.volume[1], t);
				}
			}

			{
				float left_index_f = sound->index;
				int left_index_i = floorfi(left_index_f) * 2;
				float left_sample0 = sound_sample_arr[left_index_i] / (float)c_max_s16;
				float left_sample1 = 0;
				if(left_index_i + 2 >= sound_sample_count) {
					left_sample1 = left_sample0;
				}
				else {
					left_sample1 = sound_sample_arr[left_index_i + 2] / (float)c_max_s16;
				}
				float value = lerp(left_sample0, left_sample1, fract(sound->index));
				value_left += value * sound->data.volume * fade_volume;
			}

			{
				float right_index_f = sound->index + 1;
				int right_index_i = floorfi(right_index_f) * 2 + 1;
				float right_sample0 = sound_sample_arr[right_index_i] / (float)c_max_s16;
				float right_sample1 = 0;
				if(right_index_i + 2 >= sound_sample_count) {
					right_sample1 = right_sample0;
				}
				else {
					right_sample1 = sound_sample_arr[right_index_i + 2] / (float)c_max_s16;
				}
				float value = lerp(right_sample0, right_sample1, fract(sound->index));
				value_right += value * sound->data.volume * fade_volume;
			}

			sound->index += sound->data.speed;
			if(floorfi(sound->index) * 2 >= sound_sample_count) {
				if(sound->data.loop) {
					sound->index -= sound_sample_count / 2;
				}
				else {
					g_platform_data.active_sound_arr.remove_and_swap(sound_i);
					sound_i -= 1;
				}
			}
		}
		ptr[0] = value_left;
		ptr[1] = value_right;
		ptr += 2;
	}
}

func void play_sound(e_sound sound_id, s_play_sound_data data)
{
	Mix_Chunk* chunk = g_platform_data.sound_arr[sound_id];
	if(!g_platform_data.active_sound_arr.is_full()) {
		s_active_sound active_sound = zero;
		active_sound.chunk = chunk;
		active_sound.data = data;
		g_platform_data.active_sound_arr.add(active_sound);
	}
	// Mix_PlayChannel(-1, chunk, 0);
}
