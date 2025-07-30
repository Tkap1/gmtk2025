


struct s_string_parse
{
	b8 success;
	char* first_char;
	char* last_char;
	char* continuation;
	char* result;
	int count;
};

enum e_json
{
	e_json_object,
	e_json_integer,
	e_json_string,
	e_json_bool,
	e_json_array,
	e_json_null,
};

struct s_json
{
	e_json type;
	char* key;
	s_json* next;

	union
	{
		b8 bool_val;
		s_json* object;
		s_json* array;
		int integer;
		char* str;
	};
};

struct s_leaderboard_entry
{
	int rank;
	int time;
	s_str_builder<32> nice_name;
	s_str_builder<32> internal_name;
};
