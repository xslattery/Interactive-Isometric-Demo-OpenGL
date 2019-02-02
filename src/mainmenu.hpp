

struct Main_Menu {
	
	Packed_Glyph_Texture pgt;
	Text_Mesh tm = {0};
	Text_Mesh tm_play = {0};
	Text_Mesh tm_options = {0};
	unsigned int tm_shader_id;

	Nine_Sliced_Sprite nine_sliced_sprite;

	Nine_Sliced_Sprite nine_sliced_sprite_play;
	glm::vec4 play_button_color;

	Nine_Sliced_Sprite nine_sliced_sprite_options;
	glm::vec4 options_button_color;
	unsigned int ts_shader_id;
	unsigned int ts_texture_id;

	unsigned int nsbb_texture_id_play;
	unsigned int nsbb_texture_id_options;
	unsigned int nsbb_texture_id_not_pressed;
	unsigned int nsbb_texture_id_pressed;

	Nine_Sliced_Sprite border;
	unsigned int border_texture_id;

	bool main_menu_enabled = true;
	bool exit_main_menu = false;
	float exit_countdown = 0.0f;

	void init();
	void input();
	void update();
	void render();
};