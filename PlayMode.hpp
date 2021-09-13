#include "PPU466.hpp"
#include "Mode.hpp"

#include <glm/glm.hpp>

#include <vector>
#include <deque>

struct PlayMode : Mode {
	PlayMode();
	virtual ~PlayMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const &, glm::uvec2 const &window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const &drawable_size) override;

	//the uint8_t map for each elements
	uint8_t target_sprite_offset = 2;
	char mazeCharMap[31][33] = {{0}};
	std::vector<std::pair<uint8_t, uint8_t>> grounds;
	std::vector<std::pair<uint8_t, uint8_t>> stones;
	std::vector<std::pair<uint8_t, uint8_t>> targets;
	std::pair<uint8_t, uint8_t> startPoint;

	//PPU466 Index
	uint8_t ground_tile_index = 0;
	uint8_t ground_palette_index = 6;
	uint8_t stone_tile_index = 1;
	uint8_t stone_palette_index = 7;
	uint8_t player_tile_index = 2;
	uint8_t player_palette_index = 7;
	uint8_t target_tile_index = 3;
	uint8_t target_palette_index = 7;
	uint8_t button_tile_index = 4;
	uint8_t button_palette_index = 7;
	uint8_t dark_tile_index = 5;
	uint8_t dark_palette_index = 0;

	//----- game state -----

	//input tracking:
	struct Button {
		uint8_t downs = 0;
		uint8_t pressed = 0;
	} left, right, down, up;

	bool isDark = false;
	bool goToStart = false;
	uint8_t score = 0;


	//some weird background animation:
	float background_fade = 0.0f;

	//player position:
	glm::vec2 player_at = glm::vec2(0.0f);

	//----- drawing handled by PPU466 -----

	PPU466 ppu;
};
