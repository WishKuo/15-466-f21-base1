#include "PlayMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

//for glm::value_ptr() :
#include <glm/gtc/type_ptr.hpp>

#include <random>
#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <deque>

#include <iostream>
#include <fstream>

#include "load_save_png.hpp"
#include "read_write_chunk.hpp"
#include "data_path.hpp"
#include "Load.hpp"

std::ifstream tile_stream;
std::ifstream palette_stream;

Load<void> ps(LoadTagDefault, []()
			  {
				  tile_stream.open(data_path("../tiles.asset"));
				  palette_stream.open(data_path("../palettes.asset"));
				  return;
			  });

PlayMode::PlayMode()
{
	//TODO:
	// you *must* use an asset pipeline of some sort to generate tiles.
	// don't hardcode them like this!
	// or, at least, if you do hardcode them like this,
	//  make yourself a script that spits out the code that you paste in here
	//   and check that script into your repository.

	//Also, *don't* use these tiles in your game:

	//load tiles & palettes

	std::vector<PPU466::Tile> tiles;
	read_chunk(tile_stream, std::string("tile"), &tiles);

	std::vector<PPU466::Palette> palettes;
	read_chunk(palette_stream, std::string("pale"), &palettes);

	palette_stream.close();
	tile_stream.close();

	{ //use tiles 0-16 as some weird dot pattern thing:
		std::array<uint8_t, 8 * 8> distance;
		for (uint32_t y = 0; y < 8; ++y)
		{
			for (uint32_t x = 0; x < 8; ++x)
			{
				float d = glm::length(glm::vec2((x + 0.5f) - 4.0f, (y + 0.5f) - 4.0f));
				d /= glm::length(glm::vec2(4.0f, 4.0f));
				distance[x + 8 * y] = std::max(0, std::min(255, int32_t(255.0f * d)));
			}
		}

		for (int i = 0; i < tiles.size(); i++)
		{
			ppu.tile_table[32 + i] = tiles[i];
		}
		for (int i = 0; i < palettes.size(); i++)
		{
			ppu.palette_table[i] = palettes[i];
		}
	}

	// Read map .txt
	std::ifstream ifs;
	ifs.open("assets/mazeDesign.txt");
	if (!ifs.is_open())
	{
		std::cout << "Failed to open file.\n";
	}
	else
	{
		int i = 0;
		while (!ifs.eof())
		{
			ifs.getline(mazeCharMap[i], sizeof(mazeCharMap[i]));
			i += 1;
		}
		ifs.close();
	}

	// Map the read data to uint8_t map for each elements
	for (uint8_t y = 0; y < 30; y++)
	{
		for (uint8_t x = 0; x < 32; x++)
		{
			switch (mazeCharMap[y][x])
			{
			case 'o':
				grounds.push_back(std::pair<uint8_t, uint8_t>(x, y));
				break;
			case 'i':
				stones.push_back(std::pair<uint8_t, uint8_t>(x, y));
				break;
			case 't':
				targets.push_back(std::pair<uint8_t, uint8_t>(x, y));
				break;
			default:
				startPoint = std::pair<uint8_t, uint8_t>(x, y);
				break;
			}
		}
	}

	// Set up player start point
	player_at.x = startPoint.first * 8.0f;
	player_at.y = startPoint.second * 8.0f;

	{ //use tiles 0-16 as some weird dot pattern thing:
		std::array<uint8_t, 8 * 8> distance;
		for (uint32_t y = 0; y < 8; ++y)
		{
			for (uint32_t x = 0; x < 8; ++x)
			{
				float d = glm::length(glm::vec2((x + 0.5f) - 4.0f, (y + 0.5f) - 4.0f));
				d /= glm::length(glm::vec2(4.0f, 4.0f));
				distance[x + 8 * y] = std::max(0, std::min(255, int32_t(255.0f * d)));
			}
		}
		for (uint32_t index = 0; index < 16; ++index)
		{
			PPU466::Tile tile;
			uint8_t t = (255 * index) / 16;
			for (uint32_t y = 0; y < 8; ++y)
			{
				uint8_t bit0 = 0;
				uint8_t bit1 = 0;
				for (uint32_t x = 0; x < 8; ++x)
				{
					uint8_t d = distance[x + 8 * y];
					if (d > t)
					{
						bit0 |= (1 << x);
					}
					else
					{
						bit1 |= (1 << x);
					}
				}
				tile.bit0[y] = bit0;
				tile.bit1[y] = bit1;
			}
			ppu.tile_table[index] = tile;
		}
	}

	start_time = std::chrono::high_resolution_clock::now();
}

PlayMode::~PlayMode()
{
}

bool PlayMode::handle_event(SDL_Event const &evt, glm::uvec2 const &window_size)
{

	if (evt.type == SDL_KEYDOWN)
	{
		if (evt.key.keysym.sym == SDLK_LEFT)
		{
			left.downs += 1;
			left.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_RIGHT)
		{
			right.downs += 1;
			right.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_UP)
		{
			up.downs += 1;
			up.pressed = true;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_DOWN)
		{
			down.downs += 1;
			down.pressed = true;
			return true;
		}

		if (evt.key.keysym.sym == SDLK_SPACE)
		{
			goToStart = true;
		}
	}
	else if (evt.type == SDL_KEYUP)
	{
		if (evt.key.keysym.sym == SDLK_LEFT)
		{
			left.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_RIGHT)
		{
			right.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_UP)
		{
			up.pressed = false;
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_DOWN)
		{
			down.pressed = false;
			return true;
		}
	}

	return false;
}

void PlayMode::update(float elapsed)
{
	currentTimer += elapsed;

	if (isGameOver)
		return;
	//slowly rotates through [0,1):
	// (will be used to set background color)
	background_fade += elapsed / 10.0f;
	background_fade -= std::floor(background_fade);

	constexpr float PlayerSpeed = 30.0f;
	glm::vec2 newPlayerPos = glm::vec2(player_at.x, player_at.y);
	if (left.pressed)
		newPlayerPos.x -= PlayerSpeed * elapsed;
	if (right.pressed)
		newPlayerPos.x += PlayerSpeed * elapsed;
	if (down.pressed)
		newPlayerPos.y -= PlayerSpeed * elapsed;
	if (up.pressed)
		newPlayerPos.y += PlayerSpeed * elapsed;

	//reset button press counters:
	left.downs = 0;
	right.downs = 0;
	up.downs = 0;
	down.downs = 0;

	// Detect collide with stones
	int32_t playerCenterIntPos_x = (newPlayerPos.x + 4.0f) / 8.0f;
	int32_t playerCenterIntPos_y = (newPlayerPos.y + 4.0f) / 8.0f;

	if (mazeCharMap[playerCenterIntPos_y][playerCenterIntPos_x] == 'i')
	{
		newPlayerPos.x = player_at.x;
		newPlayerPos.y = player_at.y;
	}
	else if (mazeCharMap[playerCenterIntPos_y][playerCenterIntPos_x] == 't')
	{
		currentTimer -= 12.0f;
		if (currentTimer < 0.0f)
			currentTimer = 0.0f;
		mazeCharMap[playerCenterIntPos_y][playerCenterIntPos_x] = 'o';
	}

	if (mazeCharMap[playerCenterIntPos_y][playerCenterIntPos_x] == 'x')
	{
		isDark = false;
	}
	else
	{
		isDark = true;
	}

	if (goToStart)
	{
		newPlayerPos.x = startPoint.first * 8.0f;
		newPlayerPos.y = startPoint.second * 8.0f;
		goToStart = false;
	}

	player_at = newPlayerPos;
}

void PlayMode::draw(glm::uvec2 const &drawable_size)
{
	//--- set ppu state based on game state ---

	//tilemap gets recomputed every frame as some weird plasma thing:
	//NOTE: don't do this in your game! actually make a map or something :-)
	// Set up background
	for (uint32_t y = 0; y < 30; ++y)
	{
		for (uint32_t x = 0; x < 32; ++x)
		{
			if (!isDark)
			{
				switch (mazeCharMap[y][x])
				{
				case 'o':
					ppu.background[x + PPU466::BackgroundWidth * y] =
						ground_tile_index | ((ground_palette_index & 0x07) << 8);
					break;

				case 'i':
					ppu.background[x + PPU466::BackgroundWidth * y] =
						stone_tile_index | ((stone_palette_index & 0x07) << 8);
					break;

				case 'x':
					ppu.background[x + PPU466::BackgroundWidth * y] =
						button_tile_index | ((button_palette_index & 0x07) << 8);
					break;

				case 't':
					ppu.background[x + PPU466::BackgroundWidth * y] =
						target_tile_index | ((target_palette_index & 0x07) << 8);

				default:
					break;
				}
			}
			else
			{
				switch (mazeCharMap[y][x])
				{
				case 't':
					ppu.background[x + PPU466::BackgroundWidth * y] =
						target_tile_index | ((target_palette_index & 0x07) << 8);
					break;

				case 'x':
					ppu.background[x + PPU466::BackgroundWidth * y] =
						button_tile_index | ((button_palette_index & 0x07) << 8);
					break;

				default:
					ppu.background[x + PPU466::BackgroundWidth * y] =
						dark_tile_index | ((dark_palette_index & 0x07) << 8);
					break;
				}
			}
		}
	}

	// 128 sec
	if (currentTimer > 128.0f)
	{
		isGameOver = true;
	}

	for (uint32_t x = 0; x < 32 - uint32_t(currentTimer / 4.0f); ++x)
	{
		ppu.background[x] =
			player_tile_index | ((player_palette_index & 0x07) << 8);
	}

	//player sprite:
	ppu.sprites[0].x = int32_t(player_at.x);
	ppu.sprites[0].y = int32_t(player_at.y);
	ppu.sprites[0].index = player_tile_index;
	ppu.sprites[0].attributes = player_palette_index;

	//--- actually draw ---
	ppu.draw(drawable_size);
}
