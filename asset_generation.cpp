#include "GL.hpp"
#include <glm/glm.hpp>

#include <iostream>
#include <fstream>

#include "load_save_png.hpp"
#include "read_write_chunk.hpp"
#include "PPU466.hpp"
#include "data_path.hpp"

#include <string>
#include <vector>
#include <stdint.h>

int main() {
	// Referenced ideas from previous project: Drift and referenced logic from previous project: Cherry Grove

	std::vector<PPU466::Tile> tiles;
	std::vector<PPU466::Palette> palettes;
	
	std::ifstream assetfile;
	glm::uvec2 size;
	std::vector<glm::u8vec4> data;
	assetfile.open(data_path("../ast_list.txt"));

	std::string line;
	if (assetfile.is_open()) {
		while (std::getline(assetfile, line)) {
			load_png(line, &size, &data, LowerLeftOrigin);
			int width = size[0] / 8;
			int height = size[1] / 8;
			int color_num = 0;
			PPU466::Palette p;

			for (int i=0; i < width; i++) {
				for (int j=0; j < height; j++) {

					// create one tile
					PPU466::Tile t = {
						//{ 0, 0, 0, 0, 0, 0, 0, 0 },  
						{ 0, 0, 0, 0, 0, 0, 0, 0 } // we change our format for one 8*8 pixel tile only
					};
					for (int a=0; a < 8; a++) {
						for (int b=0; b < 8; b++) {
							// pixel (a,b) in the block; a rows; b columns
							int index = (j * 8 + b) * width * 8 + i * 8 + a;
							int there = -1;
							glm::u8vec4 cur_color = data[index];

							for (int m=0; m < color_num; m++) {
								if (p[m][0] == cur_color[0] && p[m][1] == cur_color[1] &&
									p[m][2] == cur_color[2] && p[m][3] == cur_color[3]) {
									there = m;
									break;
								}
							}
							if (there >= 0) {
								// set tile
								t.bit0[b] = t.bit0[b] | ((there & 1) << a);
								t.bit1[b] = t.bit1[b] | (((there >> 1) & 1) << a);
							} else {
								// set color
								p[color_num] = cur_color;
								// set tile bits
								t.bit0[b] = t.bit0[b] | ((color_num & 1) << a);
								t.bit1[b] = t.bit1[b] | (((color_num >> 1) & 1) << a);
								color_num++;
							}
						}
					}
					tiles.push_back(t);
				}
			}
			palettes.push_back(p);
		}
		assetfile.close();

		std::ofstream tiles_stream;
		std::ofstream palettes_stream;
		
		palettes_stream.open(data_path("../palettes.asset"));
		tiles_stream.open(data_path("../tiles.asset"));
		//palettes_stream.open(data_path("../p2.asset"));
		//tiles_stream.open(data_path("../t2.asset"));

		write_chunk(std::string("tile"), tiles, &tiles_stream);
		write_chunk(std::string("pale"), palettes, &palettes_stream);
		
		tiles_stream.close();
		palettes_stream.close();
		
	}

}
