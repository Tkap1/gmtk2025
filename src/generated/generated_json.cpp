func s_string_parse parse_string(char* str, b8 do_alloc);
func char* parse_json_key(char** out_str);
func s_json* parse_json_object(char** out_str);
func s_json* parse_json(char* str);
func void print_json(s_json* json);
func s_json* json_get(s_json* json, char* key_name, e_json in_type);
