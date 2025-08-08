
#if defined(__EMSCRIPTEN__)
#include "generated/generated_leaderboard.cpp"
#endif

func void parse_leaderboard_json(s_json* json)
{
	s_json* temp = json_get(json, "items", e_json_array);
	if(!temp) { goto end; }
	temp = json_get(json, "items", e_json_array);
	for(s_json* j = temp->array; j; j = j->next) {
		if(j->type != e_json_object) { continue; }

		s_leaderboard_entry entry = {};
		s_json* player = json_get(j->object, "player", e_json_object)->object;

		entry.rank = json_get(j->object, "rank", e_json_integer)->integer;

		char* nice_name = json_get(player, "name", e_json_string)->str;
		if(nice_name) {
			cstr_into_builder(&entry.nice_name, nice_name);
		}

		char* internal_name = json_get(player, "public_uid", e_json_string)->str;
		cstr_into_builder(&entry.internal_name, internal_name);

		entry.time = json_get(j->object, "score", e_json_integer)->integer;
		game->leaderboard_arr.add(entry);
	}
	end:;
}

func void load_or_create_leaderboard_id()
{
	#if defined(__EMSCRIPTEN__)
	printf("%s\n", __FUNCTION__);
	emscripten_idb_async_load("leaderboard", "id", null, on_leaderboard_id_load_success, on_leaderboard_id_load_error);
	#elif defined(m_winhttp)

	{
		s_len_str server = S("api.lootlocker.io");
		s_len_str resource = S("/game/v2/session/guest");
		s_len_str body = S("{\"game_key\": \"dev_ae7c0ca6ad2047e1890f76fe7836a5e3\", \"game_version\": \"0.0.0.1\", \"development_mode\": true}");
		s_http_request result = do_post_request(game->session, server, resource, body, &game->circular_arena);
		assert(is_http_request_ok(result.status_code));
		assert(result.memory.size > 0);
		s_json* json = parse_json((char*)result.memory.ptr);
		s_json* temp = json_get(json, "session_token", e_json_string);
		assert(temp);
		cstr_into_builder(&game->leaderboard_session_token, temp->str);
	}

	#endif
}

#if defined(__EMSCRIPTEN__)

func void on_store_success(void* arg)
{
	(void)arg;
	printf("stored\n");
}

func void on_store_error(void* arg)
{
	(void)arg;
	printf("error\n");
}

func void on_leaderboard_id_load_success(void* arg, void* in_data, int data_len)
{
	printf("%s\n", __FUNCTION__);
	char data[1024] = {};
	memcpy(data, in_data, data_len);

	emscripten_fetch_attr_t attr = {};
	emscripten_fetch_attr_init(&attr);
	strcpy(attr.requestMethod, "POST");
	attr.onsuccess = register_leaderboard_client_success;
	attr.onerror = failure;
	attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;

	s_len_str body = format_text2("{\"game_key\": \"dev_ae7c0ca6ad2047e1890f76fe7836a5e3\", \"player_identifier\": \"%s\", \"game_version\": \"0.0.0.1\", \"development_mode\": true}", (char*)data);
	attr.requestData = body.str;
	attr.requestDataSize = body.count;
	emscripten_fetch(&attr, "https://api.lootlocker.io/game/v2/session/guest");
}

func void on_leaderboard_id_load_error(void* arg)
{
	printf("%s\n", __FUNCTION__);
	emscripten_fetch_attr_t attr = {};
	emscripten_fetch_attr_init(&attr);
	strcpy(attr.requestMethod, "POST");
	attr.onsuccess = register_leaderboard_client_success;
	attr.onerror = failure;
	attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;

	const char* body = "{\"game_key\": \"dev_ae7c0ca6ad2047e1890f76fe7836a5e3\", \"game_version\": \"0.0.0.1\", \"development_mode\": true}";
	attr.requestData = body;
	attr.requestDataSize = strlen(body);
	emscripten_fetch(&attr, "https://api.lootlocker.io/game/v2/session/guest");
}

func void register_leaderboard_client_success(emscripten_fetch_t *fetch)
{
	printf("%s\n", __FUNCTION__);
	char buffer[4096] = {};
	memcpy(buffer, fetch->data, fetch->numBytes);
	s_json* json = parse_json(buffer);
	s_json* temp = json_get(json, "session_token", e_json_string);
	if(temp) {
		cstr_into_builder(&game->leaderboard_session_token, temp->str);
		temp = json_get(json, "public_uid", e_json_string);
		cstr_into_builder(&game->leaderboard_public_uid, temp->str);
		char* player_identifier = json_get(json, "player_identifier", e_json_string)->str;
		char* nice_name = json_get(json, "player_name", e_json_string)->str;
		cstr_into_builder(&game->leaderboard_player_identifier, player_identifier);
		if(nice_name) {
			cstr_into_builder(&game->leaderboard_nice_name, nice_name);
		}
		game->leaderboard_player_id = json_get(json, "player_id", e_json_integer)->integer;
	}
	emscripten_idb_async_store("leaderboard", "id", (void*)game->leaderboard_player_identifier.str, game->leaderboard_player_identifier.count, null, on_store_success, on_store_error);
	// We're done with the fetch, so free it
	emscripten_fetch_close(fetch);
}

func void failure(emscripten_fetch_t *fetch)
{
	emscripten_fetch_close(fetch);
}

func void get_our_leaderboard(int leaderboard_id)
{
	printf("%s\n", __FUNCTION__);

	if(game->leaderboard_session_token.count <= 0) { return; }

	{
		emscripten_fetch_attr_t attr = {};
		emscripten_fetch_attr_init(&attr);
		strcpy(attr.requestMethod, "GET");
		attr.onsuccess = get_our_leaderboard_success;
		attr.onerror = failure;
		attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;

		char* headers[] = {"x-session-token", builder_to_cstr(&game->leaderboard_session_token, &game->circular_arena), NULL};
		attr.requestHeaders = headers;
		s_len_str url = format_text2("https://api.lootlocker.io/game/leaderboards/%i/member/%i", leaderboard_id, game->leaderboard_player_id);
		emscripten_fetch(&attr, url.str);
	}
}

func void when_leaderboard_obtained(s_json* json)
{
	printf("%s\n", __FUNCTION__);

	game->leaderboard_arr.count = 0;
	parse_leaderboard_json(json);
	get_our_leaderboard(c_leaderboard_id);
	game->leaderboard_received = true;
}

func void when_our_leaderboard_obtained(s_json* json)
{
	printf("%s\n", __FUNCTION__);

	s_json* j = json->object;
	if(!j) { return; }

	s_leaderboard_entry new_entry = {};
	s_json* player = json_get(j, "player", e_json_object);
	if(!player) { return; }
	player = player->object;

	new_entry.rank = json_get(j, "rank", e_json_integer)->integer;

	char* nice_name = json_get(player, "name", e_json_string)->str;
	if(nice_name) {
		cstr_into_builder(&new_entry.nice_name, nice_name);
	}

	char* internal_name = json_get(player, "public_uid", e_json_string)->str;

	cstr_into_builder(&new_entry.internal_name, internal_name);

	new_entry.time = json_get(j, "score", e_json_integer)->integer;

	// @Note(tkap, 05/06/2024): We are not in this leaderboard!
	if(new_entry.rank <= 0 || new_entry.time <= 0) {
		return;
	}

	b8 is_already_in_top_ten = false;
	foreach_val(entry_i, entry, game->leaderboard_arr) {
		if(strcmp(internal_name, entry.internal_name.str) == 0) {
			is_already_in_top_ten = true;
			break;
		}
	}

	if(!is_already_in_top_ten) {
		game->leaderboard_arr.add(new_entry);
	}
}


func void get_leaderboard_success(emscripten_fetch_t *fetch)
{
	printf("%s\n", __FUNCTION__);

	char* data = (char*)malloc(fetch->numBytes + 1);
	memcpy(data, fetch->data, fetch->numBytes);
	data[fetch->numBytes] = '\0';
	emscripten_fetch_close(fetch);

	s_json* json = parse_json(data);
	when_leaderboard_obtained(json);
	free(data);
}

func void get_our_leaderboard_success(emscripten_fetch_t *fetch)
{
	printf("%s\n", __FUNCTION__);

	char* data = (char*)malloc(fetch->numBytes + 1);
	memcpy(data, fetch->data, fetch->numBytes);
	data[fetch->numBytes] = '\0';
	emscripten_fetch_close(fetch);

	s_json* json = parse_json(data);
	when_our_leaderboard_obtained(json);
	free(data);
}

func void submit_leaderboard_success(emscripten_fetch_t *fetch)
{
	printf("%s\n", __FUNCTION__);

	emscripten_fetch_close(fetch);
	when_leaderboard_score_submitted();
}

func void submit_leaderboard_score(int time, int leaderboard_id)
{
	printf("%s\n", __FUNCTION__);

	if(game->leaderboard_session_token.count <= 0) { return; }

	emscripten_fetch_attr_t attr = {};
	emscripten_fetch_attr_init(&attr);
	strcpy(attr.requestMethod, "POST");
	attr.onsuccess = submit_leaderboard_success;
	attr.onerror = failure;
	attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;

	s_len_str data = format_text2("{\"score\": %i}", time);
	char* headers[] = {"x-session-token", builder_to_cstr(&game->leaderboard_session_token, &game->circular_arena), NULL};
	attr.requestHeaders = headers;
	attr.requestData = data.str;
	attr.requestDataSize = data.count;
	s_len_str url = format_text2("https://api.lootlocker.io/game/leaderboards/%i/submit", leaderboard_id);
	emscripten_fetch(&attr, url.str);
}

func void set_leaderboard_name_success(emscripten_fetch_t *fetch)
{
	printf("%s\n", __FUNCTION__);
	on_set_leaderboard_name(true);
	emscripten_fetch_close(fetch);
}

func void set_leaderboard_name_fail(emscripten_fetch_t *fetch)
{
	printf("%s\n", __FUNCTION__);
	on_set_leaderboard_name(false);
	emscripten_fetch_close(fetch);
}


func char* to_cstr(s_len_str str, s_linear_arena* arena)
{
	char* result = (char*)arena_alloc(arena, str.count + 1);
	memcpy(result, str.str, str.count);
	result[str.count] = 0;
	return result;
}

func void set_leaderboard_name(s_len_str name)
{
	printf("%s\n", __FUNCTION__);

	assert(game->leaderboard_session_token.count > 0);

	emscripten_fetch_attr_t attr = {};
	emscripten_fetch_attr_init(&attr);
	strcpy(attr.requestMethod, "PATCH");
	attr.onsuccess = set_leaderboard_name_success;
	attr.onerror = set_leaderboard_name_fail;
	attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;

	s_len_str data = format_text2("{\"name\": \"%s\"}", to_cstr(name, &game->update_frame_arena));
	char* headers[] = {"x-session-token", builder_to_cstr(&game->leaderboard_session_token, &game->circular_arena), NULL};
	attr.requestHeaders = headers;
	attr.requestData = data.str;
	attr.requestDataSize = data.count;
	s_len_str url = S("https://api.lootlocker.io/game/player/name");
	emscripten_fetch(&attr, url.str);
}

func void on_set_leaderboard_name(b8 success)
{
	if(success) {
		add_state(&game->state0, e_game_state0_win_leaderboard);
		submit_leaderboard_score(game->hard_data.update_count, c_leaderboard_id);
		game->update_count_at_win_time = game->hard_data.update_count;
	}
	else {
		cstr_into_builder(&game->input_name_state.error_str, "Name is already taken!");
	}
}

func void when_leaderboard_score_submitted()
{
	get_leaderboard(c_leaderboard_id);
}

#endif

func b8 get_leaderboard(int leaderboard_id)
{
	printf("%s\n", __FUNCTION__);

	game->leaderboard_received = false;
	game->leaderboard_arr.count = 0;

	if(game->leaderboard_session_token.count <= 0) { return false; }

	#if defined(__EMSCRIPTEN__)

	{
		emscripten_fetch_attr_t attr = {};
		emscripten_fetch_attr_init(&attr);
		strcpy(attr.requestMethod, "GET");
		attr.onsuccess = get_leaderboard_success;
		attr.onerror = failure;
		attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;

		char* headers[] = {"x-session-token", builder_to_cstr(&game->leaderboard_session_token, &game->circular_arena), NULL};
		attr.requestHeaders = headers;
		s_len_str url = format_text2("https://api.lootlocker.io/game/leaderboards/%i/list?count=10", leaderboard_id);
		emscripten_fetch(&attr, url.str);
	}
	#elif defined(m_winhttp)

	{
		s_len_str server = S("api.lootlocker.io");
		s_len_str resource = format_text("/game/leaderboards/%i/list?count=10", leaderboard_id);
		s_len_str token = builder_to_len_str(&game->leaderboard_session_token);
		s_len_str headers = format_text2("x-session-token: %.*s", expand_str(token));
		s_http_request result = do_get_request(game->session, server, resource, headers, &game->circular_arena);
		assert(is_http_request_ok(result.status_code));
		s_json* json = parse_json((char*)result.memory.ptr);

		game->leaderboard_arr.count = 0;
		parse_leaderboard_json(json);
		// get_our_leaderboard(c_leaderboard_id);
		game->leaderboard_received = true;
	}

	#endif
	return true;
}
