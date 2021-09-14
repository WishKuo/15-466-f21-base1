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

PlayMode::PlayMode()
{
	//TODO:
	// you *must* use an asset pipeline of some sort to generate tiles.
	// don't hardcode them like this!
	// or, at least, if you do hardcode them like this,
	//  make yourself a script that spits out the code that you paste in here
	//   and check that script into your repository.

	//Also, *don't* use these tiles in your game:

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

	//use tile 0 as a "ground":
	ppu.tile_table[0].bit0 = {
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
	};
	ppu.tile_table[0].bit1 = {
		0b00000000,
		0b00000000,
		0b00000000,
		0b00000000,
		0b00000000,
		0b00000000,
		0b00000000,
		0b00000000,
	};

	//use tile 1 as a "stone":
	ppu.tile_table[1].bit0 = {
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
	};
	ppu.tile_table[1].bit1 = {
		0b00000000,
		0b00000000,
		0b00000000,
		0b00000000,
		0b00000000,
		0b00000000,
		0b00000000,
		0b00000000,
	};

	//use tile 2 as a "player":
	ppu.tile_table[2].bit0 = {
		0b01111110,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b01111110,
	};
	ppu.tile_table[2].bit1 = {
		0b00000000,
		0b00000000,
		0b00011000,
		0b00100100,
		0b00000000,
		0b00100100,
		0b00000000,
		0b00000000,
	};

	//use tile 3 as a "target":
	ppu.tile_table[3].bit0 = {
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
	};
	ppu.tile_table[3].bit1 = {
		0b00000000,
		0b01000010,
		0b00100100,
		0b00100100,
		0b00111100,
		0b00100100,
		0b00100100,
		0b01000010,
	};

	//use tile 4 as a "button":
	ppu.tile_table[4].bit0 = {
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
	};
	ppu.tile_table[4].bit1 = {
		0b00000000,
		0b00000000,
		0b00000000,
		0b00111100,
		0b00111100,
		0b00111100,
		0b00000000,
		0b00000000,
	};

	//use tile 5 as a "dark":
	ppu.tile_table[5].bit0 = {
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
		0b11111111,
	};
	ppu.tile_table[5].bit1 = {
		0b00000000,
		0b00000000,
		0b00000000,
		0b00000000,
		0b00000000,
		0b00000000,
		0b00000000,
		0b00000000,
	};

	// use as the timer
	ppu.tile_table[6].bit0 = {
		0b11111111,
		0b00000000,
		0b00000000,
		0b00000000,
		0b00000000,
		0b00000000,
		0b00000000,
		0b11111111,
	};

	ppu.tile_table[6].bit1 = {
		0b00000000,
		0b00000000,
		0b00000000,
		0b00000000,
		0b00000000,
		0b00000000,
		0b00000000,
		0b00000000,
	};

	//makes the outside of tiles 0-16 solid:
	ppu.palette_table[0] = {
		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
	};

	//makes the center of tiles 0-16 solid:
	ppu.palette_table[1] = {
		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
	};

	//used for the player:
	ppu.palette_table[7] = {
		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
		glm::u8vec4(0xff, 0xff, 0x00, 0xff),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
	};

	//used for the misc other sprites:
	ppu.palette_table[6] = {
		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
		glm::u8vec4(0x88, 0x88, 0xff, 0xff),
		glm::u8vec4(0x00, 0x00, 0x00, 0xff),
		glm::u8vec4(0x00, 0x00, 0x00, 0x00),
	};

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
		score++;
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

	// glm::vec2 button_leftDownCorner = glm::vec2(startPoint.first * 8.0f, startPoint.second * 8.0f);
	// glm::vec2 button_rightTopCorner = glm::vec2(startPoint.first * 8.0f + 8.0f, startPoint.second * 8.0f + 8.0f);
	// if (player_at.x >= button_leftDownCorner.x && player_at.x <= button_rightTopCorner.x &&
	// 	player_at.y >= button_leftDownCorner.y && player_at.y <= button_rightTopCorner.y)
	// {
	// 	isDark = false;
	// }
	// else
	// {
	// 	isDark = true;
	// }
}

void PlayMode::draw(glm::uvec2 const &drawable_size)
{
	//--- set ppu state based on game state ---

	//background color will be some hsv-like fade:
	ppu.background_color = glm::u8vec4(
		std::min(255, std::max(0, int32_t(255 * 0.5f * (0.5f + std::sin(2.0f * M_PI * (background_fade + 0.0f / 3.0f)))))),
		std::min(255, std::max(0, int32_t(255 * 0.5f * (0.5f + std::sin(2.0f * M_PI * (background_fade + 1.0f / 3.0f)))))),
		std::min(255, std::max(0, int32_t(255 * 0.5f * (0.5f + std::sin(2.0f * M_PI * (background_fade + 2.0f / 3.0f)))))),
		0xff);

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

	end_time = std::chrono::high_resolution_clock::now();
	total_time = end_time - start_time;
	int currentTimer = floor(total_time.count());
	//std::cout << currentTimer << std::endl;

	// 128 sec
	if (currentTimer == 128)
	{
		isGameOver = true;
	}

	for (uint32_t x = 0; x < 32 - (currentTimer / 4.0); ++x)
	{
		ppu.background[x] =
			button_tile_index | ((button_palette_index & 0x07) << 8);
	}

	//background scroll:
	// ppu.background_position.x = int32_t(-0.5f * player_at.x);
	// ppu.background_position.y = int32_t(-0.5f * player_at.y);

	// //targets sprite:
	// for (uint32_t i  = 0; i < targets.size(); i++)
	// {
	// 	uint32_t currentIdex = i + target_sprite_offset;
	// 	ppu.sprites[currentIdex].x = targets[i].first * 8.0f;
	// 	ppu.sprites[currentIdex].y = targets[i].second * 8.0f;
	// 	ppu.sprites[currentIdex].index = target_tile_index;
	// 	ppu.sprites[currentIdex].attributes = target_palette_index;
	// }

	//player sprite:
	ppu.sprites[0].x = int32_t(player_at.x);
	ppu.sprites[0].y = int32_t(player_at.y);
	ppu.sprites[0].index = player_tile_index;
	ppu.sprites[0].attributes = player_palette_index;

	//some other misc sprites:
	// for (uint32_t i = 1; i < 63; ++i)
	// {
	// 	float amt = (i + 2.0f * background_fade) / 62.0f;
	// 	ppu.sprites[i].x = int32_t(0.5f * PPU466::ScreenWidth + std::cos(2.0f * M_PI * amt * 5.0f + 0.01f * player_at.x) * 0.4f * PPU466::ScreenWidth);
	// 	ppu.sprites[i].y = int32_t(0.5f * PPU466::ScreenHeight + std::sin(2.0f * M_PI * amt * 3.0f + 0.01f * player_at.y) * 0.4f * PPU466::ScreenWidth);
	// 	ppu.sprites[i].index = 32;
	// 	ppu.sprites[i].attributes = 6;
	// 	if (i % 2)
	// 		ppu.sprites[i].attributes |= 0x80; //'behind' bit
	// }

	//--- actually draw ---
	ppu.draw(drawable_size);

	// draw timer
	/*
	std::vector<Vertex> vertices;
	auto draw_rectangle = [&vertices](glm::vec2 const &center, glm::vec2 const &radius, glm::u8vec4 const &color)
	{
		//draw rectangle as two CCW-oriented triangles:
		vertices.emplace_back(glm::vec3(center.x - radius.x, center.y - radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x + radius.x, center.y - radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x + radius.x, center.y + radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));

		vertices.emplace_back(glm::vec3(center.x - radius.x, center.y - radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x + radius.x, center.y + radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
		vertices.emplace_back(glm::vec3(center.x - radius.x, center.y + radius.y, 0.0f), color, glm::vec2(0.5f, 0.5f));
	};
#define HEX_TO_U8VEC4(HX) (glm::u8vec4((HX >> 24) & 0xff, (HX >> 16) & 0xff, (HX >> 8) & 0xff, (HX)&0xff))
	const glm::u8vec4 timer_color = HEX_TO_U8VEC4(0xed7e7eff);
#undef HEX_TO_U8VEC4

	draw_rectangle(timer, timer_radius, timer_color);
	*/
}
