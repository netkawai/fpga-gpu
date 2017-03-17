// Written by:
//     Evan Andersen
//
// Created March 22, 2013

#include <stdint.h>
#include "math.h"
#include "gpu_driver.h"
#include "system.h"

void gradientSection(uint16_t *addr, int x, int y, int width, int height) {
	width += x;
	height += y;
	int size = 50;
	
	int white = COLOR(31,31,31);
	int black = COLOR(0,0,0);
	if((*SW >> 8) & 1) {
		white = COLOR(0, 31, 0);
		black = COLOR(0, 0, 31);
	}

	for(int i = y; i < height; i++) {
		for(int j = x; j < width; j++) {
			int color;
			if(((i/size) % 2) == ((j/size) % 2)) {
				color = black;
			} else {
				color = white;
			}
			addr[i*800 + j] = color;
		}
	}
}

void gradient(uint16_t *addr) {
	gradientSection(addr, 0, 0, 800, 600);
}


void flush_dcache() {
  char* i;
  for (i = (char*) 0; i < (char*) 32768; i+= 32)
  { 
    __asm__ volatile ("flushd (%0)" :: "r" (i));
  }
}

void fail(uint32_t value) {
	while(1) {
		int newValue = value;
		if(*SW & 2) {
			 newValue >>= 16;
		}
		*LEDR = newValue & 0xffff;
	}
}

void memtest32(uint32_t *addr, uint32_t size) {
	//write a pattern in
	int pattern = 0x0fefefef;
	for(int i = 0; i < size; i++) {
		if(i % (1024*1024) == 0) {
			*LEDR = i/(1024*1024);
		}
		addr[i] = i + pattern;
	}
	//test it
	for(int i = 0; i < size; i++) {
		if(i % (1024*1024) == 0) {
			*LEDR = i/(1024*1024);
		}
		uint32_t value = addr[i] - i;
		if(value != pattern) {
			fail(i);
		}
	}
}
 

// void draw_triangle_barycentric(uint16_t* addr, point_t v0, point_t v1, point_t v2, uint16_t color) {
	
    // // Compute triangle bounding box
    // int minX = min3(v0.x, v1.x, v2.x);
    // int minY = min3(v0.y, v1.y, v2.y);
    // int maxX = max3(v0.x, v1.x, v2.x);
    // int maxY = max3(v0.y, v1.y, v2.y);
	
	// //coordinates in 1/16ths of a pixel
	// int subMask = SUBSTEP - 1;

	// minX = (minX + subMask) & ~subMask;
    // minY = (minY + subMask) & ~subMask;

	// //dont mult by SUBSTEP for hw
	// int16_t A01 = (v0.y - v1.y);
	// int16_t B01 = (v1.x - v0.x);
	
    // int16_t A12 = (v1.y - v2.y);
	// int16_t B12 = (v2.x - v1.x);
	
    // int16_t A20 = (v2.y - v0.y);
	// int16_t B20 = (v0.x - v2.x);

    // // Barycentric coordinates at minX/minY corner
    // point p = { minX, minY };
    // int w0_row = orient2d(v1, v2, p);
    // int w1_row = orient2d(v2, v0, p);
    // int w2_row = orient2d(v0, v1, p);

	// //Fill rules
	// w0_row -= isTopLeft(v1, v2) ? 0 : 1;
    // w1_row -= isTopLeft(v2, v0) ? 0 : 1;
    // w2_row -= isTopLeft(v0, v1) ? 0 : 1;
	
    // // Rasterize	
    // for (p.y = minY; p.y <= maxY; p.y += SUBSTEP) {
        // // Barycentric coordinates at start of row
		// int w0 = w0_row;
		// int w1 = w1_row;
		// int w2 = w2_row;

        // for (p.x = minX; p.x <= maxX; p.x += SUBSTEP) {
            // // If p is on or inside all edges, render pixel.
            // if ((w0 | w1 | w2) >= 0) {
				// uint16_t x = p.x / 16;
				// uint16_t y = p.y / 16;
                // addr[y*800 + x] = color;        
			// }
            // // One step to the right
            // w0 += A12*SUBSTEP;
            // w1 += A20*SUBSTEP;
            // w2 += A01*SUBSTEP;
        // }

        // // One row step
        // w0_row += B12*SUBSTEP;
        // w1_row += B20*SUBSTEP;
        // w2_row += B01*SUBSTEP;
    // }
// }

void *memset(void *ptr, int val, size_t n) {
	for(size_t i = 0; i < n; i++) {
		((char*)ptr)[i] = val;
	}
	return ptr;
}

void draw_checkerboard_gpu(uint16_t *addr) {
	//reset the sequence number
	GPU[0] = 5;
	
	uint32_t gpu_seq = 0;
	//set the stride
	GPU[9] = 800*2;
	gpu_seq++;
	
	uint16_t tile_height = ((600 + 31)/32);
	uint16_t tile_width = ((800 + 31)/32);
	for(int i = 0; i < tile_height; i++) {
		for(int j = 0; j < tile_width; j++) {
			//clear tile
			
			color_t color;
			if((i % 2) == (j % 2)) {
				color = COLOR(0, 0, 0);
			} else {
				color = COLOR(31, 31, 31);
			}

			GPU[1] = color;

			GPU[2] = 0;
			GPU[3] = 0;
			GPU[4] = 0;
			GPU[10] = 0;
			GPU[11] = 0;
			GPU[12] = 0;
			
			GPU[5] = 1;
			GPU[6] = 1;
			GPU[7] = 1;
			
			GPU[8] = ((uint32_t)(addr)) + (i*32*800*2) + (j*32*2);
						
			//start rendering tile
			GPU[0] = 0;
			gpu_seq += 12;
			
			//write it to fifo
			GPU[0] = 2;
			GPU[0] = 4;
			gpu_seq += 2;
		}
	}	
	//wait for all data to finish writing
	GPU[0] = 3;
	GPU[0] = 4;
	gpu_seq += 2;
	while(GPU[0] < gpu_seq);

}

void gpu_test(uint16_t *addr, color_t color) {
	//reset the sequence number
	GPU[0] = 5;
	
	uint32_t gpu_seq = 0;
	//set the stride
	GPU[9] = 800*2;
	gpu_seq++;
	
	//clear tile
	GPU[1] = color;

	GPU[2] = 0;
	GPU[3] = 0;
	GPU[4] = 0;
	GPU[10] = 0;
	GPU[11] = 0;
	GPU[12] = 0;
	
	GPU[5] = 1;
	GPU[6] = 1;
	GPU[7] = 1;
	
	GPU[8] = ((uint32_t)(addr));
				
	//start rendering tile
	GPU[0] = 0;
	gpu_seq += 12;
	
	//write it to fifo
	GPU[0] = 2;
	gpu_seq++;

	//wait for all data to finish writing
	GPU[0] = 3;
	GPU[0] = 4;
	gpu_seq += 2;
	uint32_t seq_no = 0;
	while(seq_no < gpu_seq) {
		seq_no = GPU[0];
		*LEDR = seq_no;
	}

}


int main(void) {
	uint16_t *frontFB = (uint16_t*)(DRAM);
	uint16_t *backFB = (uint16_t*)(DRAM + 243200);

	
	color_t lightBlue = COLOR(13,20,31);
	color_t yellow = COLOR(31,31,0);
	color_t red = COLOR(31,0,0);
	color_t green = COLOR(0,29,5);
	color_t purple = COLOR(15,0,19);
	color_t orange = COLOR(31,16,0);
	
	VGA[1] = (uint32_t)frontFB;
	gpu_test(frontFB, orange);
	while(*SW & 0x2);
	gpu_test(frontFB+32, lightBlue);
	while(*SW & 0x4);
	
	// int32_t pos = 0;
	// int32_t angle = 180;
	// uint32_t width = 50;
	// uint32_t distance = 600 - width*2;
	int32_t xpos = 0;
	int32_t ypos = 0;
	int32_t zpos = 0;
	while(1) {
		if(*KEY & 0x1) {
			xpos += 1;
			xpos %= 360;
		}
		if(*KEY & 0x2) {
			ypos += 1;
			ypos %= 360;
		}
		if(*KEY & 0x4) {
			zpos += 1;
			zpos %= 360;
		}
		// uint32_t y = pos;
		// if(y > distance) {
			// y = distance*2 - y;
		// }
		// y += width;
		
		// uint32_t keys = *KEY;
		// if(keys & 0x4) {
			// pos += 2;
			// pos %= distance*2;
		// }
		
		// if(keys & 0x1) {
			// angle += 1;
			// if(angle >= 360) {
				// angle -= 360;
			// }
		// } else if(keys & 0x2) {
			// angle -= 1;
			// if(angle < 0) {
				// angle += 360;
			// }
		// }
		
		//uint32_t y = 100;
		// float rad = (360-angle)*F_2_PI/360.0f;
		// float cosT = cos(rad);
		// float sinT = sin(rad);
		
		point_t vertices[] = {
			{.x = -0.2, .y = -0.2, .z = 0.2},
			{.x = 0.2,  .y = -0.2, .z = 0.2},
			{.x = -0.2, .y = 0.2,  .z = 0.2},
			{.x = 0.2,  .y = 0.2,  .z = 0.2},
			{.x = -0.2, .y = -0.2, .z = -0.2},
			{.x = 0.2,  .y = -0.2, .z = -0.2},
			{.x = -0.2, .y = 0.2,  .z = -0.2},
			{.x = 0.2,  .y = 0.2,  .z = -0.2},

			{.x = 0.6, .y = -0.2, .z = 0.2},
			{.x = 1.0,  .y = -0.2, .z = 0.2},
			{.x = 0.6, .y = 0.2,  .z = 0.2},
			{.x = 1.0,  .y = 0.2,  .z = 0.2},
			{.x = 0.6, .y = -0.2, .z = -0.2},
			{.x = 1.0,  .y = -0.2, .z = -0.2},
			{.x = 0.6, .y = 0.2,  .z = -0.2},
			{.x = 1.0,  .y = 0.2,  .z = -0.2},
		};
		if(ypos < 180) {
			vertices = {
				{.x = 0.6, .y = -0.2, .z = 0.2},
				{.x = 1.0,  .y = -0.2, .z = 0.2},
				{.x = 0.6, .y = 0.2,  .z = 0.2},
				{.x = 1.0,  .y = 0.2,  .z = 0.2},
				{.x = 0.6, .y = -0.2, .z = -0.2},
				{.x = 1.0,  .y = -0.2, .z = -0.2},
				{.x = 0.6, .y = 0.2,  .z = -0.2},
				{.x = 1.0,  .y = 0.2,  .z = -0.2},

				{.x = -0.2, .y = -0.2, .z = 0.2},
				{.x = 0.2,  .y = -0.2, .z = 0.2},
				{.x = -0.2, .y = 0.2,  .z = 0.2},
				{.x = 0.2,  .y = 0.2,  .z = 0.2},
				{.x = -0.2, .y = -0.2, .z = -0.2},
				{.x = 0.2,  .y = -0.2, .z = -0.2},
				{.x = -0.2, .y = 0.2,  .z = -0.2},
				{.x = 0.2,  .y = 0.2,  .z = -0.2},
			};
		}

		int32_t nvertices = sizeof(vertices)/sizeof(point_t);

		triangle_t tris[] =  {
			{.v0 = 2, .v1 = 0, .v2 = 1, .color = yellow},
			{.v0 = 2, .v1 = 1, .v2 = 3, .color = yellow},
			{.v0 = 3, .v1 = 1, .v2 = 7, .color = lightBlue},
			{.v0 = 1, .v1 = 5, .v2 = 7, .color = lightBlue},
			{.v0 = 6, .v1 = 2, .v2 = 7, .color = red},
			{.v0 = 2, .v1 = 3, .v2 = 7, .color = red},
			{.v0 = 6, .v1 = 4, .v2 = 2, .color = green},
			{.v0 = 4, .v1 = 0, .v2 = 2, .color = green},
			{.v0 = 0, .v1 = 4, .v2 = 1, .color = orange},
			{.v0 = 4, .v1 = 5, .v2 = 1, .color = orange},
			{.v0 = 6, .v1 = 7, .v2 = 4, .color = purple},
			{.v0 = 5, .v1 = 4, .v2 = 7, .color = purple},
			
			{.v0 = 10, .v1 = 8, .v2 = 9, .color = yellow},
			{.v0 = 10, .v1 = 9, .v2 = 11, .color = yellow},
			{.v0 = 11, .v1 = 9, .v2 = 15, .color = lightBlue},
			{.v0 = 9, .v1 = 13, .v2 = 15, .color = lightBlue},
			{.v0 = 14, .v1 = 10, .v2 = 15, .color = red},
			{.v0 = 10, .v1 = 11, .v2 = 15, .color = red},
			{.v0 = 14, .v1 = 12, .v2 = 10, .color = green},
			{.v0 = 12, .v1 = 8, .v2 = 10, .color = green},
			{.v0 = 8, .v1 = 12, .v2 = 9, .color = orange},
			{.v0 = 12, .v1 = 13, .v2 = 9, .color = orange},
			{.v0 = 14, .v1 = 15, .v2 = 12, .color = purple},
			{.v0 = 13, .v1 = 12, .v2 = 15, .color = purple},
		};
		int32_t ntris = sizeof(tris)/sizeof(triangle_t);
		
		render_target_t target = {
			.framebuffer = backFB,
			.stride = 800*2,
			.width = 800,
			.height = 600,
		};
				
		float xAngle = (xpos/360.0f)*F_2_PI;
		float sinX = sine(xAngle);
		float cosX = cosine(xAngle);
		float rotMatX[] = {
			1, 0, 0, 0,
			0, cosX, -sinX, 0,
			0, sinX, cosX, 0,
			0, 0, 0, 1,
		};
		float yAngle = (ypos/360.0f)*F_2_PI;
		float sinY = sine(yAngle);
		float cosY = cosine(yAngle);
		float rotMatY[] = {
			cosY, 0, sinY, 0,
			0, 1, 0, 0,
			-sinY, 0, cosY, 0,
			0, 0, 0, 1,
		};
		float zAngle = (zpos/360.0f)*F_2_PI;
		float sinZ = sine(zAngle);
		float cosZ = cosine(zAngle);
		float rotMatZ[] = {
			cosZ, -sinZ, 0, 0,
			sinZ, cosZ, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1,
		};

		float viewMat[] = {
			0.5, 0, 0, 0,
			0, 0.5, 0, 0,
			0, 0, 0.5, -0.7,
			0, 0, 0, 1,			
		};
		
		float near = 0.1;
		float far = 100;
		float top = 0.1;
		float bottom = -top;
		float right = (4.0f/3.0f) * top;
		float left = -right;
		
		float tmp[16];
		scene_t scene = {
			.background = COLOR(0, 0, 0),
			.poly_list = {
				.vertices = vertices,
				.triangles = tris,
				.nvertices = nvertices,
				.ntris = ntris,
			},
			//perspective
			.proj = {
				2*near/(right-left), 0, (right+left)/(right-left), 0,
				0, 2*near/(top-bottom), (top+bottom)/(top-bottom), 0,
				0, 0, -(far+near)/(far-near), -(2*far*near)/(far-near),
				0, 0, -1, 0,			
			},
			
			//ortho
			// .proj = {
				// 2/(right-left), 0, 0, -(right+left)/(right-left),
				// 0, 2/(top-bottom), 0, -(top+bottom)/(top-bottom),
				// 0, 0, -2/(far-near), -(far+near)/(far-near),
				// 0, 0, 0, 1,			
			// },
		};
		mult_4x4(rotMatY, rotMatX, scene.view);
		mult_4x4(rotMatZ, scene.view, tmp);
		mult_4x4(viewMat, tmp, scene.view);

		if(SW[0] & 0x2) {
			gradient(frontFB);
			flush_dcache();
			while(SW[0] & 0x2);
		}
		if(SW[0] & 0x4) {
			draw_checkerboard_gpu(frontFB);
			while(SW[0] & 0x4);
		}
		
		//time for the magic
		gpu_render_scene(&target, &scene);
				
		//send the fresh frame to the VGA module
		VGA[1] = (uint32_t)backFB;
		
		//wait for the VGA module to start using the new buffer
		while(VGA[1] != (uint32_t)backFB);
		
		//how many times did the screen draw while we rendered this frame?
		*LEDR = VGA[0];
		
		//swap back/front buffers 
		uint16_t *tmpFB = backFB;
		backFB = frontFB;
		frontFB = tmpFB;
	}
	return 0;
}
