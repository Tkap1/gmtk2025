func s_loaded_sound load_sound_from_file(char* path);
func void init_common();
func void my_audio_callback(void* userdata, u8* stream, int len);
func void play_sound(e_sound sound_id, s_play_sound_data data);
