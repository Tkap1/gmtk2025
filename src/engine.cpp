


func s_render_flush_data make_render_flush_data(s_v3 cam_pos, s_v3 player_pos)
{
	s_render_flush_data result = zero;
	result.view = m4_identity();
	result.projection = m4_identity();
	result.light_view = m4_identity();
	result.light_projection = m4_identity();
	result.cam_pos = cam_pos;
	result.cull_mode = e_cull_mode_disabled;
	result.blend_mode = e_blend_mode_normal;
	result.depth_mode = e_depth_mode_read_and_write;
	result.player_pos = player_pos;
	return result;
}

func s_mesh make_mesh_from_vertices(s_vertex* vertex_arr, int vertex_count)
{
	s_mesh result = zero;
	gl(glGenVertexArrays(1, &result.vao));
	gl(glBindVertexArray(result.vao));

	int attrib_index = 0;
	{
		gl(glGenBuffers(1, &result.vertex_vbo));
		gl(glBindBuffer(GL_ARRAY_BUFFER, result.vertex_vbo));

		u8* offset = 0;
		constexpr int stride = sizeof(float) * 12;

		gl(glVertexAttribPointer(attrib_index, 3, GL_FLOAT, GL_FALSE, stride, offset)); // pos
		gl(glEnableVertexAttribArray(attrib_index));
		attrib_index += 1;
		offset += sizeof(float) * 3;

		gl(glVertexAttribPointer(attrib_index, 3, GL_FLOAT, GL_FALSE, stride, offset)); // normal
		gl(glEnableVertexAttribArray(attrib_index));
		attrib_index += 1;
		offset += sizeof(float) * 3;

		gl(glVertexAttribPointer(attrib_index, 4, GL_FLOAT, GL_FALSE, stride, offset)); // rgba
		gl(glEnableVertexAttribArray(attrib_index));
		attrib_index += 1;
		offset += sizeof(float) * 4;

		gl(glVertexAttribPointer(attrib_index, 2, GL_FLOAT, GL_FALSE, stride, offset)); // uv
		gl(glEnableVertexAttribArray(attrib_index));
		attrib_index += 1;
		offset += sizeof(float) * 2;

		result.vertex_count = vertex_count;
		gl(glBufferData(GL_ARRAY_BUFFER, sizeof(s_vertex) * vertex_count, vertex_arr, GL_STATIC_DRAW));
	}

	{
		gl(glGenBuffers(1, &result.instance_vbo.id));
		gl(glBindBuffer(GL_ARRAY_BUFFER, result.instance_vbo.id));

		u8* offset = 0;
		constexpr int stride = sizeof(float) * 24;

		gl(glVertexAttribPointer(attrib_index, 4, GL_FLOAT, GL_FALSE, stride, offset)); // instance color
		gl(glEnableVertexAttribArray(attrib_index));
		gl(glVertexAttribDivisor(attrib_index, 1));
		attrib_index += 1;
		offset += sizeof(float) * 4;

		gl(glVertexAttribPointer(attrib_index, 2, GL_FLOAT, GL_FALSE, stride, offset)); // uv min
		gl(glEnableVertexAttribArray(attrib_index));
		gl(glVertexAttribDivisor(attrib_index, 1));
		attrib_index += 1;
		offset += sizeof(float) * 2;

		gl(glVertexAttribPointer(attrib_index, 2, GL_FLOAT, GL_FALSE, stride, offset)); // uv max
		gl(glEnableVertexAttribArray(attrib_index));
		gl(glVertexAttribDivisor(attrib_index, 1));
		attrib_index += 1;
		offset += sizeof(float) * 2;

		gl(glVertexAttribPointer(attrib_index, 4, GL_FLOAT, GL_FALSE, stride, offset)); // mat4_0
		gl(glEnableVertexAttribArray(attrib_index));
		gl(glVertexAttribDivisor(attrib_index, 1));
		attrib_index += 1;
		offset += sizeof(float) * 4;

		gl(glVertexAttribPointer(attrib_index, 4, GL_FLOAT, GL_FALSE, stride, offset)); // mat4_1
		gl(glEnableVertexAttribArray(attrib_index));
		gl(glVertexAttribDivisor(attrib_index, 1));
		attrib_index += 1;
		offset += sizeof(float) * 4;

		gl(glVertexAttribPointer(attrib_index, 4, GL_FLOAT, GL_FALSE, stride, offset)); // mat4_2
		gl(glEnableVertexAttribArray(attrib_index));
		gl(glVertexAttribDivisor(attrib_index, 1));
		attrib_index += 1;
		offset += sizeof(float) * 4;

		gl(glVertexAttribPointer(attrib_index, 4, GL_FLOAT, GL_FALSE, stride, offset)); // mat4_3
		gl(glEnableVertexAttribArray(attrib_index));
		gl(glVertexAttribDivisor(attrib_index, 1));
		attrib_index += 1;
		offset += sizeof(float) * 4;
	}
	return result;
}

func s_ply_mesh parse_ply_mesh(char* path, s_linear_arena* arena)
{
	char* data = (char*)read_file(path, arena);
	char* cursor = data;
	s_ply_mesh result = zero;
	b8 has_color = strstr(cursor, "property uchar red") != null;

	{
		char* temp = strstr(cursor, "element vertex ") + 15;
		result.vertex_count = atoi(temp);
		assert(result.vertex_count <= c_max_vertices);
	}
	{
		char* temp = strstr(cursor, "element face ") + 13;
		result.face_count = atoi(temp);
		assert(result.face_count <= c_max_faces);
	}
	cursor = strstr(data, "end_header") + 11;

	{
		u8* temp = (u8*)cursor;
		for(int i = 0; i < result.vertex_count; i += 1) {
			memcpy(&result.vertex_arr[i].pos, temp, sizeof(s_v3));
			temp += sizeof(s_v3);
			memcpy(&result.vertex_arr[i].normal, temp, sizeof(s_v3));
			temp += sizeof(s_v3);
			if(has_color) {
				result.vertex_arr[i].color.x = (*temp) / 255.0f;
				temp += 1;
				result.vertex_arr[i].color.y = (*temp) / 255.0f;
				temp += 1;
				result.vertex_arr[i].color.z = (*temp) / 255.0f;
				temp += 1;
				result.vertex_arr[i].color.w = (*temp) / 255.0f;
				temp += 1;
			}
			else {
				result.vertex_arr[i].color = make_color(1);
			}
			memcpy(&result.vertex_arr[i].uv, temp, sizeof(s_v2));
			temp += sizeof(s_v2);
		}
		cursor = (char*)temp;
	}
	{
		s_ply_face* temp = (s_ply_face*)cursor;
		for(int i = 0; i < result.face_count; i += 1) {
			result.face_arr[i] = *temp;
			assert(result.face_arr[i].index_count == 3);
			temp += 1;
		}
	}
	return result;
}

func s_mesh make_mesh_from_ply_file(char* file, s_linear_arena* arena)
{
	s_ply_mesh ply_mesh = parse_ply_mesh(file, arena);
	int vertex_count = ply_mesh.face_count * 3;
	assert(vertex_count < c_max_vertices);
	s_vertex vertex_arr[c_max_vertices] = zero;
	for(int i = 0; i < ply_mesh.face_count; i += 1) {
		vertex_arr[i * 3 + 0] = ply_mesh.vertex_arr[ply_mesh.face_arr[i].index_arr[0]];
		vertex_arr[i * 3 + 1] = ply_mesh.vertex_arr[ply_mesh.face_arr[i].index_arr[1]];
		vertex_arr[i * 3 + 2] = ply_mesh.vertex_arr[ply_mesh.face_arr[i].index_arr[2]];
	}
	s_mesh result = make_mesh_from_vertices(vertex_arr, vertex_count);
	return result;
}

func s_mesh make_mesh_from_obj_file(char* file, s_linear_arena* arena)
{
	s_obj_mesh* obj_mesh = parse_obj_mesh(file, arena);
	int vertex_count = obj_mesh->face_count * 3;
	assert(vertex_count < c_max_vertices);
	s_vertex vertex_arr[c_max_vertices] = zero;
	for(int i = 0; i < obj_mesh->face_count; i += 1) {
		for(int j = 0; j < 3; j += 1) {
			vertex_arr[i * 3 + j].pos = obj_mesh->vertex_arr[obj_mesh->face_arr[i].vertex_index[j] - 1].pos;
			vertex_arr[i * 3 + j].normal = obj_mesh->normal_arr[obj_mesh->face_arr[i].normal_index[j] - 1];
			vertex_arr[i * 3 + j].uv = obj_mesh->uv_arr[obj_mesh->face_arr[i].uv_index[j] - 1];
			vertex_arr[i * 3 + j].color.rgb = obj_mesh->vertex_arr[obj_mesh->face_arr[i].vertex_index[j] - 1].color;
			vertex_arr[i * 3 + j].color.a = 1;
		}
	}
	s_mesh result = make_mesh_from_vertices(vertex_arr, vertex_count);
	return result;
}

func void set_cull_mode(e_cull_mode mode)
{
	switch(mode) {
		case e_cull_mode_disabled: {
			glDisable(GL_CULL_FACE);
		} break;

		case e_cull_mode_back_ccw: {
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			glFrontFace(GL_CCW);
		} break;

		case e_cull_mode_front_ccw: {
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
			glFrontFace(GL_CCW);
		} break;

		case e_cull_mode_back_cw: {
			glEnable(GL_CULL_FACE);
			glCullFace(GL_BACK);
			glFrontFace(GL_CW);
		} break;

		case e_cull_mode_front_cw: {
			glEnable(GL_CULL_FACE);
			glCullFace(GL_FRONT);
			glFrontFace(GL_CW);
		} break;
		invalid_default_case;
	}
}

func void set_depth_mode(e_depth_mode mode)
{
	switch(mode) {
		case e_depth_mode_no_read_no_write: {
			glDisable(GL_DEPTH_TEST);
			glDepthMask(GL_FALSE);
		} break;
		case e_depth_mode_read_and_write: {
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LESS);
			glDepthMask(GL_TRUE);
		} break;
		case e_depth_mode_read_no_write: {
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LESS);
			glDepthMask(GL_FALSE);
		} break;
		case e_depth_mode_no_read_yes_write: {
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_ALWAYS);
			glDepthMask(GL_TRUE);
		} break;
		invalid_default_case;
	}
}

func void set_blend_mode(e_blend_mode mode)
{
	switch(mode) {
		case e_blend_mode_disabled: {
			glDisable(GL_BLEND);
		} break;
		case e_blend_mode_additive: {
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
		} break;
		case e_blend_mode_premultiply_alpha: {
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		} break;
		case e_blend_mode_multiply: {
			glEnable(GL_BLEND);
			glBlendFunc(GL_ZERO, GL_SRC_COLOR);
		} break;
		case e_blend_mode_multiply_inv: {
			glEnable(GL_BLEND);
			// glBlendFunc(GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
			glBlendFuncSeparate(GL_ZERO, GL_ONE_MINUS_SRC_COLOR, GL_ZERO, GL_ONE);
		} break;
		case e_blend_mode_normal: {
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		} break;
		case e_blend_mode_additive_no_alpha: {
			glEnable(GL_BLEND);
			glBlendFuncSeparate(GL_ONE, GL_ONE, GL_ZERO, GL_ONE);
		} break;
		invalid_default_case;
	}
}

func void set_window_size(int width, int height)
{
	SDL_SetWindowSize(g_platform_data->window, width, height);
}

func s_texture load_texture_from_file(char* path, u32 filtering)
{
	int width, height, num_channels;
	void* data = stbi_load(path, &width, &height, &num_channels, 4);
	assert(data);

	s_texture result = load_texture_from_data(data, width, height, GL_RGBA, filtering);
	return result;
}

func s_texture load_texture_from_data(void* data, int width, int height, int format, u32 filtering)
{
	s_texture result = zero;

	gl(glGenTextures(1, &result.id));
	gl(glBindTexture(GL_TEXTURE_2D, result.id));
	gl(glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data));
	gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
	gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
	gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filtering));
	gl(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filtering));
	gl(glGenerateMipmap(GL_TEXTURE_2D));

	return result;
}

// @TODO(tkap, 13/10/2024): premultiply??
func s_font load_font_from_file(char* file, int font_size, s_linear_arena* arena)
{
	u8* file_data = read_file(file, arena);
	s_font font = {};
	font.size = (float)font_size;

	assert(file_data);

	stbtt_fontinfo info = {};
	stbtt_InitFont(&info, file_data, 0);

	stbtt_GetFontVMetrics(&info, &font.ascent, &font.descent, &font.line_gap);

	font.scale = stbtt_ScaleForPixelHeight(&info, (float)font_size);
	constexpr int max_chars = 128;
	int bitmap_count = 0;
	u8* bitmap_arr[max_chars];
	const int padding = 10;

	int columns = floorfi((float)(4096 / (font_size + padding)));
	int rows = ceilfi((max_chars - columns) / (float)columns) + 1;

	int total_width = floorfi((float)(columns * (font_size + padding)));
	// @Note(tkap, 20/10/2023): We need to align the texture width to 4 bytes! Very important! Thanks to tk_dev
	total_width = (total_width + 3) & ~3;
	int total_height = floorfi((float)(rows * (font_size + padding)));

	for(int char_i = 0; char_i < max_chars; char_i++)
	{
		s_glyph glyph = {};
		u8* bitmap = stbtt_GetCodepointBitmap(&info, 0, font.scale, char_i, &glyph.width, &glyph.height, 0, 0);
		stbtt_GetCodepointBox(&info, char_i, &glyph.x0, &glyph.y0, &glyph.x1, &glyph.y1);
		stbtt_GetGlyphHMetrics(&info, char_i, &glyph.advance_width, NULL);

		font.glyph_arr[char_i] = glyph;
		bitmap_arr[bitmap_count++] = bitmap;
	}

	u8* gl_bitmap = (u8*)arena_alloc_zero(arena, sizeof(u8) * 1 * total_width * total_height);

	for(int char_i = 0; char_i < max_chars; char_i++) {
		s_glyph* glyph = &font.glyph_arr[char_i];
		u8* bitmap = bitmap_arr[char_i];
		int column = char_i % columns;
		int row = char_i / columns;
		for(int y = 0; y < glyph->height; y++) {
			for(int x = 0; x < glyph->width; x++) {
				int current_x = floorfi((float)(column * (font_size + padding)));
				int current_y = floorfi((float)(row * (font_size + padding)));
				u8 src_pixel = bitmap[x + y * glyph->width];
				u8* dst_pixel = &gl_bitmap[((current_x + x) + (current_y + y) * total_width)];
				dst_pixel[0] = src_pixel;
			}
		}

		glyph->uv_min.x = column / (float)columns;
		glyph->uv_max.x = glyph->uv_min.x + (glyph->width / (float)total_width);

		// @Hack(tkap, 07/04/2025): Without this letters are cut off on the left side
		glyph->uv_min.x -= 0.001f;

		glyph->uv_min.y = row / (float)rows;

		// @Note(tkap, 17/05/2023): For some reason uv_max.y is off by 1 pixel (checked the texture in renderoc), which causes the text to be slightly miss-positioned
		// in the Y axis. "glyph->height - 1" fixes it.
		glyph->uv_max.y = glyph->uv_min.y + (glyph->height / (float)total_height);

		// @Note(tkap, 17/05/2023): Otherwise the line above makes the text be cut off at the bottom by 1 pixel...
		// glyph->uv_max.y += 0.01f;
	}

	for(int bitmap_i = 0; bitmap_i < bitmap_count; bitmap_i++) {
		stbtt_FreeBitmap(bitmap_arr[bitmap_i], NULL);
	}

	game->texture_arr[e_texture_font] = load_texture_from_data(gl_bitmap, total_width, total_height, m_gl_single_channel, GL_LINEAR);

	return font;
}

func s_v2 draw_text(s_len_str text, s_v2 in_pos, float font_size, s_v4 color, b8 centered, s_font* font)
{
	float scale = font->scale * (font_size / font->size);

	assert(text.count > 0);
	if(centered) {
		s_v2 text_size = get_text_size(text, font, font_size);
		in_pos.x -= text_size.x / 2;
		in_pos.y -= text_size.y / 2;
	}
	s_v2 pos = in_pos;
	pos.y += font->ascent * scale;

	s_text_iterator it = {};
	while(iterate_text(&it, text, color)) {
		for(int char_i = 0; char_i < it.text.count; char_i++) {
			int c = it.text[char_i];
			if(c <= 0 || c >= 128) { continue; }

			if(c == '\n' || c == '\r') {
				pos.x = in_pos.x;
				pos.y += font_size;
				continue;
			}

			s_glyph glyph = font->glyph_arr[c];
			s_v2 draw_size = v2((glyph.x1 - glyph.x0) * scale, (glyph.y1 - glyph.y0) * scale);

			s_v2 glyph_pos = pos;
			glyph_pos.x += glyph.x0 * scale;
			glyph_pos.y += -glyph.y0 * scale;

			// t.flags |= e_render_flag_use_texture | e_render_flag_text;
			s_v3 tpos = v3(glyph_pos, 0.0f);

			s_v2 center = tpos.xy + draw_size / 2 * v2(1, -1);
			s_v2 bottomleft = tpos.xy;


			s_m4 model = m4_translate(v3(tpos.xy, 0));
			model = m4_multiply(model, m4_scale(v3(draw_size, 1)));

			// t.color = it.color;
			s_v2 uv_min = glyph.uv_min;
			s_v2 uv_max = glyph.uv_max;
			// swap(&uv_min.y, &uv_max.y);
			// t.origin_offset = c_origin_bottomleft;

			s_v4 temp_color = it.color;
			temp_color.a = color.a;
			draw_texture_screen(tpos.xy, draw_size, temp_color, e_texture_font, e_shader_text, uv_min, uv_max);

			// draw_generic(game_renderer, &t, render_pass, render_data.shader, font->texture.game_id, e_mesh_rect);

			pos.x += glyph.advance_width * scale;

		}
	}

	return v2(pos.x, in_pos.y);
}

// @TODO(tkap, 31/10/2023): Handle new lines
func s_v2 get_text_size_with_count(s_len_str in_text, s_font* font, float font_size, int count, int in_column)
{
	assert(count >= 0);
	if(count <= 0) { return {}; }

	int column = in_column;

	s_v2 size = {};
	float max_width = 0;
	float scale = font->scale * (font_size / font->size);
	size.y = font_size;

	s_len_str text = substr_from_to_exclusive(in_text, 0, count);
	s_text_iterator it = {};
	while(iterate_text(&it, text, make_color(0))) {
		for(int char_i = 0; char_i < it.text.count; char_i++) {
			char c = it.text[char_i];
			s_glyph glyph = font->glyph_arr[c];
			if(c == '\t') {
				int spaces = get_spaces_for_column(column);
				size.x += glyph.advance_width * scale * spaces;
				column += spaces;
			}
			else if(c == '\n') {
				size.y += font_size;
				size.x = 0;
				column = 0;
			}
			else {
				size.x += glyph.advance_width * scale;
				column += 1;
			}
			max_width = max(size.x, max_width);
		}
	}
	size.x = max_width;

	return size;
}

func s_v2 get_text_size(s_len_str text, s_font* font, float font_size)
{
	return get_text_size_with_count(text, font, font_size, text.count, 0);
}

func b8 iterate_text(s_text_iterator* it, s_len_str text, s_v4 color)
{
	if(it->index >= text.count) { return false; }

	if(it->color_stack.count <= 0) {
		it->color_stack.add(color);
	}

	it->color = it->color_stack.get_last();

	int index = it->index;
	int advance = 0;
	while(index < text.count) {
		char c = text[index];
		char next_c = index < text.count - 1 ? text[index + 1] : 0;
		if(c == '$' && next_c == '$') {
			s_len_str red_str = substr_from_to_exclusive(text, index + 2, index + 4);
			s_len_str green_str = substr_from_to_exclusive(text, index + 4, index + 6);
			s_len_str blue_str = substr_from_to_exclusive(text, index + 6, index + 8);
			float red = hex_str_to_int(red_str) / 255.0f;
			float green = hex_str_to_int(green_str) / 255.0f;
			float blue = hex_str_to_int(blue_str) / 255.0f;
			s_v4 temp_color = make_color(red, green, blue);
			it->color_stack.add(temp_color);

			if(index == it->index) {
				index += 8;
				it->index += 8;
				it->color = it->color_stack.get_last();
				continue;
			}
			else {
				advance = 8;
				break;
			}
		}
		else if(c == '$' && next_c == '.') {
			if(index == it->index) {
				it->color_stack.pop_last();
				it->color = it->color_stack.get_last();
				index += 2;
				it->index += 2;
				continue;
			}
			else {
				advance = 2;
				it->color_stack.pop_last();
				break;
			}
		}
		index += 1;
	}
	it->text = substr_from_to_exclusive(text, it->index, index);
	it->index = index + advance;
	return true;
}


func int hex_str_to_int(s_len_str str)
{
	int result = 0;
	int tens = 0;
	for(int i = str.count - 1; i >= 0; i -= 1) {
		char c = str[i];
		int val = 0;
		if(is_number(c)) {
			val = c - '0';
		}
		else if(c >= 'a' && c <= 'f') {
			val = c - 'a' + 10;
		}
		else if(c >= 'A' && c <= 'F') {
			val = c - 'A' + 10;
		}
		result += val * (int)powf(16, (float)tens);
		tens += 1;
	}
	return result;
}

func s_len_str format_text(const char* text, ...)
{
	static constexpr int max_format_text_buffers = 16;
	static constexpr int max_text_buffer_length = 512;

	static char buffers[max_format_text_buffers][max_text_buffer_length] = {};
	static int index = 0;

	char* current_buffer = buffers[index];
	memset(current_buffer, 0, max_text_buffer_length);

	va_list args;
	va_start(args, text);
	#ifdef m_debug
	int written = vsnprintf(current_buffer, max_text_buffer_length, text, args);
	assert(written > 0 && written < max_text_buffer_length);
	#else
	vsnprintf(current_buffer, max_text_buffer_length, text, args);
	#endif
	va_end(args);

	index += 1;
	if(index >= max_format_text_buffers) { index = 0; }

	return S(current_buffer);
}


func u8* try_really_hard_to_read_file(char* file, s_linear_arena* arena)
{
	u8* result = null;
	for(int i = 0; i < 100; i += 1) {
		result = read_file(file, arena);
		if(result) {
			break;
		}
		SDL_Delay(10);
	}
	return result;
}

func float update_time_to_render_time(float time, float interp_dt)
{
	float result = time + (float)c_update_delay * interp_dt;
	return result;
}

func s_m4 fullscreen_m4()
{
	s_m4 result = m4_translate(v3(c_world_center, 0.0f));
	result = m4_multiply(result, m4_scale(v3(c_world_size, 0.0f)));
	return result;
}

func s_time_format update_time_to_time_format(float update_time)
{
	s_time_format result = zero;
	float milliseconds = update_time * 1000;

	result.hours = floorfi(milliseconds / 1000 / 60 / 60);
	milliseconds -= result.hours * 1000 * 60 * 60;

	result.minutes = floorfi(milliseconds / 1000 / 60);
	milliseconds -= result.minutes * 1000 * 60;

	result.seconds = floorfi(milliseconds / 1000);
	milliseconds -= result.seconds * 1000;

	result.milliseconds = floorfi(milliseconds);

	return result;
}

func s_time_format update_count_to_time_format(int update_count)
{
	float update_time = update_count * (float)c_update_delay;
	s_time_format result = update_time_to_time_format(update_time);
	return result;
}

func s_obj_mesh* parse_obj_mesh(char* path, s_linear_arena* arena)
{
	s_obj_mesh* result = (s_obj_mesh*)arena_alloc_zero(arena, sizeof(s_obj_mesh));
	char* data = (char*)read_file(path, arena);
	assert(data);
	char* cursor = strstr(data, "\nv ") + 1;
	while(memcmp(cursor, "v ", 2) == 0) {
		cursor += 2;
		char* end = null;
		result->vertex_arr[result->vertex_count].pos.x = strtof(cursor, &end);
		assert(end > cursor);
		cursor = end;
		result->vertex_arr[result->vertex_count].pos.y = strtof(cursor, &end);
		assert(end > cursor);
		cursor = end;
		result->vertex_arr[result->vertex_count].pos.z = strtof(cursor, &end);
		assert(end > cursor);
		cursor = end;

		result->vertex_arr[result->vertex_count].color.r = strtof(cursor, &end);
		if(end > cursor) {
			cursor = end;
			result->vertex_arr[result->vertex_count].color.g = strtof(cursor, &end);
			assert(end > cursor);
			cursor = end;
			result->vertex_arr[result->vertex_count].color.b = strtof(cursor, &end);
			assert(end > cursor);
			cursor = end;
		}
		else {
			result->vertex_arr[result->vertex_count].color = v3(1, 1, 1);
		}
		result->vertex_count += 1;

		while(*cursor == '\n' || *cursor == '\r' || *cursor == ' ') {
			cursor += 1;
		}
	}

	while(memcmp(cursor, "vn ", 3) == 0) {
		cursor += 3;
		char* end = null;
		result->normal_arr[result->normal_count].x = strtof(cursor, &end);
		assert(end > cursor);
		cursor = end;
		result->normal_arr[result->normal_count].y = strtof(cursor, &end);
		assert(end > cursor);
		cursor = end;
		result->normal_arr[result->normal_count].z = strtof(cursor, &end);
		assert(end > cursor);
		cursor = end;
		result->normal_count += 1;

		while(*cursor == '\n' || *cursor == '\r' || *cursor == ' ') {
			cursor += 1;
		}
	}

	while(memcmp(cursor, "vt ", 3) == 0) {
		cursor += 3;
		char* end = null;
		result->uv_arr[result->uv_count].x = strtof(cursor, &end);
		assert(end > cursor);
		cursor = end;
		result->uv_arr[result->uv_count].y = strtof(cursor, &end);
		assert(end > cursor);
		cursor = end;
		result->uv_count += 1;

		while(*cursor == '\n' || *cursor == '\r' || *cursor == ' ') {
			cursor += 1;
		}
	}

	while(memcmp(cursor, "f ", 2) != 0) {
		cursor += 1;
	}

	while(memcmp(cursor, "f ", 2) == 0) {
		cursor += 2;
		char* end = null;

		for(int i = 0; i < 3; i += 1) {
			result->face_arr[result->face_count].vertex_index[i] = (int)strtol(cursor, &end, 10);
			assert(end > cursor);
			cursor = end + 1;
			result->face_arr[result->face_count].uv_index[i] = (int)strtol(cursor, &end, 10);
			assert(end > cursor);
			cursor = end + 1;
			result->face_arr[result->face_count].normal_index[i] = (int)strtol(cursor, &end, 10);
			assert(end > cursor);
			cursor = end;

			while(*cursor == '\n' || *cursor == '\r' || *cursor == ' ') {
				cursor += 1;
			}
		}
		result->face_count += 1;
		assert(result->face_count < c_max_faces);

	}

	return result;
}

func char* skip_whitespace(char* str)
{
	while(true) {
		if(*str == '0') { break; }
		else if(*str <= ' ') { str += 1; }
		else { break; }
	}
	return str;
}

func s_v2 wxy(float x, float y)
{
	s_v2 result = c_world_size * v2(x, y);
	return result;
}

func s_v2 wxy(float x)
{
	s_v2 result = c_world_size * v2(x, x);
	return result;
}

func s_v2 wcxy(float x, float y)
{
	s_v2 result = c_world_center * v2(x, y);
	return result;
}

func void update_particles(float delta)
{
	s_soft_game_data* soft_data = &game->soft_data;

	for(int i = c_first_index[e_entity_emitter]; i < c_last_index_plus_one[e_entity_emitter]; i += 1) {
		if(!soft_data->entity_arr.active[i]) { continue; }
		s_particle_emitter_a a = soft_data->entity_arr.data[i].emitter_a;
		s_particle_emitter_b* b = &soft_data->entity_arr.data[i].emitter_b;
		s_time_data spawn_time_data = get_time_data(game->render_time, b->last_emit_timestamp, 1.0f / b->particles_per_second);

		s_time_data expire_time_data = zero;

		// @Note(tkap, 23/04/2025): Not infinite
		if(b->duration >= 0) {
			expire_time_data = get_time_data(game->render_time, b->creation_timestamp, b->duration);
		}

		while(spawn_time_data.percent >= 1 && (expire_time_data.percent < 1 || !b->existed_in_previous_frame)) {
			spawn_time_data.percent -= 1;
			b->last_emit_timestamp = game->render_time;

			if(!b->paused) {
				for(int particle_i = 0; particle_i < b->particle_count; particle_i += 1) {
					b->num_alive_particles += 1;

					s_particle particle = zero;
					particle.emitter_index = i;
					particle.seed = randu(&game->rng);
					if(!a.follow_emitter) {
						particle.pos = a.pos;
					}

					switch(b->spawn_type) {
						xcase e_emitter_spawn_type_point: {
						};

						xcase e_emitter_spawn_type_circle: {
							particle.pos.xy += random_point_in_circle(&game->rng, b->spawn_data.x);
						};

						xcase e_emitter_spawn_type_sphere: {
							particle.pos += random_point_in_sphere(&game->rng, b->spawn_data.x);
						};

						xcase e_emitter_spawn_type_rect_center: {
							particle.pos.xy += random_point_in_rect_center(&game->rng, b->spawn_data.xy);
						};

						xcase e_emitter_spawn_type_rect_edge: {
							particle.pos.xy += random_point_in_rect_edges(&game->rng, b->spawn_data.xy);
						};

						xcase e_emitter_spawn_type_circle_outline: {
							float r = randf_range(&game->rng, 0, c_tau);
							particle.pos.x += cosf(r) * b->spawn_data.x;
							particle.pos.y += sinf(r) * b->spawn_data.x;
						};
					}
					particle.spawn_timestamp = game->render_time;
					soft_data->particle_arr.add(particle);
				}
			}
		}

		if(expire_time_data.percent >= 1 && b->num_alive_particles <= 0) {
			entity_manager_remove(&soft_data->entity_arr, e_entity_emitter, i);
		}
		b->existed_in_previous_frame = true;

		if(b->pause_after_update) {
			b->paused = true;
		}
	}

	foreach_ptr(particle_i, particle, soft_data->particle_arr) {
		s_rng rng = make_rng(particle->seed);
		s_particle_emitter_a data_a = soft_data->entity_arr.data[particle->emitter_index].emitter_a;
		s_particle_emitter_b* data_b = &soft_data->entity_arr.data[particle->emitter_index].emitter_b;
		float duration = data_a.particle_duration * (1.0f - randf32(&rng) * data_a.particle_duration_rand);
		s_time_data time_data = get_time_data(game->render_time, particle->spawn_timestamp, duration);
		float radius = data_a.radius * (1.0f - randf32(&rng) * data_a.radius_rand);
		float speed = data_a.speed * (1.0f - randf32(&rng) * data_a.speed_rand);
		radius *= 1.0f - time_data.percent * data_a.shrink;

		s_v3 dir = zero;
		dir.x = data_a.dir.x * (1.0f - randf32(&rng) * data_a.dir_rand.x * 2);
		dir.y = data_a.dir.y * (1.0f - randf32(&rng) * data_a.dir_rand.y * 2);
		dir.z = data_a.dir.z * (1.0f - randf32(&rng) * data_a.dir_rand.z * 2);
		dir = v3_normalized(dir);

		s_v4 color = get_particle_color(&rng, time_data.percent, data_a.color_arr);
		color.a = 1.0f - time_data.percent;
		particle->pos += dir * speed * delta;
		particle->pos.y += time_data.passed * data_a.gravity;

		s_v3 pos = particle->pos;
		if(data_a.follow_emitter) {
			pos += data_a.pos;
		}

		{
			s_instance_data data = zero;
			data.model = m4_translate(pos);
			scale_m4_by_radius(&data.model, radius);
			data.color = color;
			add_to_render_group(data, e_shader_circle, e_texture_white, e_mesh_quad);
		}
		if(time_data.percent >= 1) {
			soft_data->particle_arr.remove_and_swap(particle_i);
			particle_i -= 1;
			data_b->num_alive_particles -= 1;
		}
	}
}

template <typename t, int n>
func int entity_manager_add(s_entity_manager<t, n>* manager, e_entity type, t new_entity)
{
	assert(manager->count[type] < n);
	int index = manager->free_list[type][manager->count[type]];
	assert(!manager->active[index]);
	manager->data[index] = new_entity;
	manager->active[index] = true;
	manager->count[type] += 1;
	return index;
}

template <typename t, int n>
func void entity_manager_remove(s_entity_manager<t, n>* manager, e_entity type, int index)
{
	assert(manager->active[index]);
	assert(manager->count[type] > 0);
	manager->count[type] -= 1;
	manager->active[index] = false;
	manager->free_list[type][manager->count[type]] = index;
}

template <typename t, int n>
func void entity_manager_reset(s_entity_manager<t, n>* manager, e_entity type)
{
	manager->count[type] = 0;
	memset(manager->active, 0, sizeof(manager->active));
	manager->free_list[type] = (int*)arena_alloc_zero(&game->arena, sizeof(int) * g_entity_type_data[type].max_count);
	for(int i = 0; i < n; i += 1) {
		manager->free_list[type][i] = c_first_index[type] + i;
	}
}

func s_v4 get_particle_color(s_rng* rng, float percent, s_list<s_particle_color, 4> color_arr)
{
	s_particle_color choice[2] = {color_arr[0], color_arr[at_most(color_arr.count - 1, 1)]};

	foreach_val(color_i, color, color_arr) {
		if(color.percent <= percent) {
			choice[0] = color;
			choice[1] = color_arr[at_most(color_arr.count - 1, color_i + 1)];
		}
	}

	float p = ilerp(choice[0].percent, choice[1].percent, percent);
	s_v4 choice2[2] = zero;

	for(int i = 0; i < 2; i += 1) {
		s_v4 temp = choice[i].color;
		if(choice[i].color_rand_per_channel) {
			float r = (1.0f - randf32(rng) * choice[i].color_rand);
			float g = (1.0f - randf32(rng) * choice[i].color_rand);
			float b = (1.0f - randf32(rng) * choice[i].color_rand);
			temp.r *= r;
			temp.g *= g;
			temp.b *= b;
		}
		else {
			float r = (1.0f - randf32(rng) * choice[i].color_rand);
			temp = multiply_rgb(choice[i].color, r);
		}
		choice2[i] = temp;
	}
	s_v4 result = lerp_color(choice2[0], choice2[1], p);
	return result;
}

func void play_sound(e_sound sound_id, s_play_sound_data data)
{
	if(!game->turn_off_all_sounds) {
		g_platform_data->play_sound(sound_id, data);
	}
}

func void timer_activate(s_timer* timer, float time_now)
{
	timer->used_timestamp = time_now;
	timer->want_to_use_timestamp = 0;
}

func b8 timer_can_activate(s_timer timer, float time_now, float duration, float cooldown)
{
	b8 result = false;
	float time_passed_since_last_activation = time_now - (timer.used_timestamp + duration);
	if(time_passed_since_last_activation >= cooldown || timer.used_timestamp <= 0) {
		result = true;
	}
	return result;
}

func b8 timer_want_activate(s_timer timer, float time_now, float grace_period)
{
	b8 result = false;
	float passed = time_now - timer.want_to_use_timestamp;
	if(passed <= grace_period && timer.want_to_use_timestamp > 0) {
		result = true;
	}
	return result;
}

func b8 timer_can_and_want_activate(s_timer timer, float time_now, float duration, float cooldown, float grace_period)
{
	b8 can = timer_can_activate(timer, time_now, duration, cooldown);
	b8 want = timer_want_activate(timer, time_now, grace_period);
	b8 result = can && want;
	return result;
}

func b8 timer_is_active(s_timer timer, float time_now, float duration)
{
	float passed = time_now - timer.used_timestamp;
	b8 result = passed <= duration && timer.used_timestamp > 0;
	return result;
}

func s_time_data timer_get_time_data(s_timer timer, float time_now, float duration, b8* active)
{
	if(active) {
		*active = timer_is_active(timer, time_now, duration);
	}
	s_time_data time_data = get_time_data(time_now, timer.used_timestamp, duration);
	return time_data;
}

func s_time_data timer_get_cooldown_time_data(s_timer timer, float time_now, float cooldown, b8* is_on_cooldown)
{
	if(is_on_cooldown) {
		*is_on_cooldown = !timer_can_activate(timer, time_now, 0.0f, cooldown);
	}
	s_time_data time_data = get_time_data(time_now, timer.used_timestamp, cooldown);
	return time_data;
}

func float get_rand_sound_speed(float x, s_rng* rng)
{
	assert(x > 1);
	float low = 1.0f / x;
	float high = x;
	float result = randf_range(rng, low, high);
	return result;
}

func s_audio_fade make_simple_fade(float p0, float p1)
{
	s_audio_fade fade = zero;
	fade.percent[0] = p0;
	fade.percent[1] = p1;
	fade.volume[0] = 1;
	fade.volume[1] = 0;
	return fade;
}