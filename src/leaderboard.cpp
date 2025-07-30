
#include "generated/generated_leaderboard.cpp"

func void on_store_success(void* arg)
{
	printf("stored\n");
}

func void on_store_error(void* arg)
{
	printf("error\n");
}

func void load_or_create_leaderboard_id()
{
	printf("%s\n", __FUNCTION__);
	emscripten_idb_async_load("leaderboard", "id", null, on_leaderboard_id_load_success, on_leaderboard_id_load_error);
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

	s_len_str body = format_text("{\"game_key\": \"dev_ae7c0ca6ad2047e1890f76fe7836a5e3\", \"player_identifier\": \"%s\", \"game_version\": \"0.0.0.1\", \"development_mode\": true}", (char*)data);
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

func char* parse_json_key(char** out_str)
{
	char* str = *out_str;
	str = skip_whitespace(str);
	assert(*str == '"');
	str += 1;
	char* start = str;
	char* key = NULL;
	while(true) {
		if(*str == '\0') { assert(false); }
		else if(*str == '"' && str[-1] != '\\') {
			u64 len = str - start;
			key = (char*)malloc(len + 1);
			memcpy(key, start, len);
			key[len] = 0;
			break;
		}
		else { str += 1; }
	}
	str += 1;
	str = skip_whitespace(str);
	assert(*str == ':');
	str += 1;
	*out_str = str;
	return key;
}

func s_json* parse_json_object(char** out_str)
{
	char* str = *out_str;
	str = skip_whitespace(str);
	s_json* result = NULL;
	if(*str == '{') {
		str += 1;
		result = (s_json*)calloc(1, sizeof(*result));
		result->type = e_json_object;
		s_json** curr = &result->object;
		while(true) {
			str = skip_whitespace(str);
			if(*str == '}') { break; }
			char* key = parse_json_key(&str);
			s_json* child = parse_json_object(&str);
			assert(child);
			child->key = key;
			*curr = child;
			curr = &child->next;
			str = skip_whitespace(str);
			if(*str != ',') { break; }
			str += 1;
		}
		str += 1;
	}
	else if(strncmp(str, "true", 4) == 0) {
		result = (s_json*)calloc(1, sizeof(*result));
		result->type = e_json_bool;
		result->bool_val = true;
		str += 4;
	}
	else if(strncmp(str, "false", 5) == 0) {
		result = (s_json*)calloc(1, sizeof(*result));
		result->type = e_json_bool;
		result->bool_val = false;
		str += 5;
	}
	else if(strncmp(str, "null", 4) == 0) {
		result = (s_json*)calloc(1, sizeof(*result));
		result->type = e_json_null;
		str += 4;
	}
	else if(*str == '"') {
		s_string_parse parse = parse_string(str, true);
		assert(parse.success);
		str = parse.continuation;
		result = (s_json*)calloc(1, sizeof(*result));
		result->type = e_json_string;
		result->str = parse.result;
	}
	else if(is_number(*str)) {
		result = (s_json*)calloc(1, sizeof(*result));
		result->type = e_json_integer;
		result->integer = atoi(str);
		while(is_number(*str)) {
			str += 1;
		}
	}
	else if('[') {
		result = (s_json*)calloc(1, sizeof(*result));
		result->type = e_json_array;
		str += 1;
		s_json** curr = &result->array;
		while(true) {
			str = skip_whitespace(str);
			if(*str == ']') { break; }
			s_json* child = parse_json_object(&str);
			assert(child);
			*curr = child;
			curr = &child->next;
			str = skip_whitespace(str);
			if(*str != ',') { break; }
			str += 1;
		}

		assert(*str == ']');
		str += 1;
	}
	*out_str = str;
	return result;
}

func s_json* parse_json(char* str)
{
	return parse_json_object(&str);
}

func void print_json(s_json* json)
{
	assert(json);
	for(s_json* j = json; j; j = j->next) {
		if(j->key) {
			printf("%s: ", j->key);
		}
		switch(j->type) {
			case e_json_bool: {
				printf("%s\n", j->bool_val ? "true" : "false");
			} break;
			case e_json_integer: {
				printf("%i\n", j->integer);
			} break;
			case e_json_string: {
				printf("\"%s\"\n", j->str);
			} break;
			case e_json_object: {
				printf("{\n");
				print_json(j->object);
				printf("}\n");
			} break;
			case e_json_array: {
				printf("[\n");
				print_json(j->array);
				printf("]\n");
			} break;
			case e_json_null: {
				printf("null\n");
			} break;
			invalid_default_case;
		}
	}
}

func s_json* json_get(s_json* json, char* key_name, e_json in_type)
{
	assert(json);
	for(s_json* j = json; j; j = j->next) {
		if(!j->key) {
			if(j->object) {
				return json_get(j->object, key_name, in_type);
			}
		}
		if(j->type == in_type && strcmp(j->key, key_name) == 0) {
			return j;
		}
	}
	return NULL;
}

func s_string_parse parse_string(char* str, b8 do_alloc)
{
	s_string_parse result = {};
	if(*str != '"') { return {}; }
	str += 1;
	result.first_char = str;
	while(true) {
		if(*str == '\0') { return {}; }
		else if(*str == '"' && str[-1] != '\\') {
			result.success = true;
			result.last_char = str - 1;
			result.count = (int)(result.last_char - result.first_char) + 1;
			result.continuation = str + 1;
			if(do_alloc && result.last_char >= result.first_char) {
				result.result = (char*)malloc(result.count + 1);
				memcpy(result.result, result.first_char, result.count);
				result.result[result.count] = 0;
			}
			break;
		}
		else { str += 1; }
	}
	return result;
}

func b8 get_leaderboard(int leaderboard_id)
{
	printf("%s\n", __FUNCTION__);

	game->leaderboard_received = false;
	game->leaderboard_arr.count = 0;

	if(game->leaderboard_session_token.count <= 0) { return false; }

	{
		emscripten_fetch_attr_t attr = {};
		emscripten_fetch_attr_init(&attr);
		strcpy(attr.requestMethod, "GET");
		attr.onsuccess = get_leaderboard_success;
		attr.onerror = failure;
		attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY;

		char* headers[] = {"x-session-token", builder_to_cstr(&game->leaderboard_session_token, &game->circular_arena), NULL};
		attr.requestHeaders = headers;
		s_len_str url = format_text("https://api.lootlocker.io/game/leaderboards/%i/list?count=10", leaderboard_id);
		emscripten_fetch(&attr, url.str);
	}
	return true;
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
		s_len_str url = format_text("https://api.lootlocker.io/game/leaderboards/%i/member/%i", leaderboard_id, game->leaderboard_player_id);
		emscripten_fetch(&attr, url.str);
	}
}

func void when_leaderboard_obtained(s_json* json)
{
	printf("%s\n", __FUNCTION__);

	game->leaderboard_arr.count = 0;
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

	s_len_str data = format_text("{\"score\": %i}", time);
	char* headers[] = {"x-session-token", builder_to_cstr(&game->leaderboard_session_token, &game->circular_arena), NULL};
	attr.requestHeaders = headers;
	attr.requestData = data.str;
	attr.requestDataSize = data.count;
	s_len_str url = format_text("https://api.lootlocker.io/game/leaderboards/%i/submit", leaderboard_id);
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

	s_len_str data = format_text("{\"name\": \"%s\"}", to_cstr(name, &game->update_frame_arena));
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