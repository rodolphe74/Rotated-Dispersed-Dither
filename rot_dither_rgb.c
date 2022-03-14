#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <float.h>

#include "cwalk/include/cwalk.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"


#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))


#define R(r,g,b) (r/255.0)
#define G(r,g,b) (g/255.0)
#define B(r,g,b) (b/255.0)

int8_t matrix[] =
{
	0,  8,	 2,    10,
	12, 4,	 14,   6,
	3,  11,	 1,    9,
	15, 7,	 13,   5
};
int32_t matrix_size[] = { 4, 4 };
int8_t matrix_max_value = 15;

typedef struct _pixel {
	short r;
	short g;
	short b;
} pixel;

typedef struct _palette {
	unsigned char colors[256][3];
	unsigned char size;
} palette;

typedef unsigned char rgb_color[3];

typedef struct _rectangle {
	int8_t *matrix;
	int32_t size[2];
} rectangle;

typedef struct _rgb {
	uint8_t *r;
	uint8_t *g;
	uint8_t *b;
} rgb;

int32_t repeat_matrix_size[] = { 8000, 8000 };
//int32_t wrapping_matrix_size[] = { 8000, 8000 };
double _A = 4.0;
double _B = 3.0;
double _C = 5.0;


int get_position(int32_t x, int32_t y, int32_t stride)
{
	return y * stride + x;
}

rectangle create_matrix(int8_t *base_matrix, int32_t xsize, int32_t ysize)
{
	int8_t *ret_matrix = calloc(xsize * ysize, sizeof(uint8_t));
	int32_t index = 0;

	for (int32_t y = 0; y < ysize; y++) {
		for (int32_t x = 0; x < xsize; x++) {
			int8_t val = base_matrix[index + (x % matrix_size[0])];
			ret_matrix[get_position(x, y, xsize)] = val;
		}

		index += matrix_size[0];
		if (index >= (matrix_size[1] * matrix_size[0])) {
			index = 0;
		}
	}
	
	rectangle rect;
	rect.matrix = ret_matrix;
	rect.size[0] = xsize;
	rect.size[1] = ysize;
	
	return rect;
}


//int compare_rectangle(int8_t *matrix, int8_t *matrix_size, int32_t x, int32_t y, int32_t x2,
//		      int32_t y2, int32_t size_x, int32_t size_y)
//{
//	for (int yy = 0; yy < size_y; yy++) {
//		for (int xx = 0; xx < size_x; xx++) {
//
//			if (matrix[get_position(x + xx, y + yy, matrix_size[0])] == -1) return 0;
//			if ((x + xx) > matrix_size[0] || (x2 + xx) > matrix_size[0]) return 0;
//			if ((y + yy) > matrix_size[1] || (y2 + yy) > matrix_size[1]) return 0;
//
//			if (matrix[get_position(x + xx, y + yy, matrix_size[0])]
//			    != matrix[get_position(x2 + xx, y2 + yy, matrix_size[0])]) {
//				return 0;
//			}
//		}
//	}
//	return 1;
//}





rectangle center_matrix(rectangle rect, int32_t size_x, int32_t size_y)
{
	int32_t center_x = rect.size[0] / 2;
	int32_t center_y = rect.size[1] / 2;
	int8_t *new_matrix = calloc(size_x * size_y, sizeof(uint8_t));

	memset(new_matrix, -1, size_x * size_y);

	int32_t x_start = size_x / 2 - center_x;
	int32_t y_start = size_y / 2 - center_y;
	int32_t xc = 0;
	int32_t yc = 0;

	for (int32_t y = y_start; y < y_start + rect.size[1]; y++) {
		for (int32_t x = x_start; x < x_start + rect.size[0]; x++) {
			// flip matrix vertically (0,0 is bottom left for discrete rotation)
			new_matrix[get_position(x, y, size_y)] = rect.matrix[get_position(xc, rect.size[1] - 1 - yc, rect.size[0])];
			xc++;
		}
		yc++;
		xc = 0;
	}

	rectangle ret;
	ret.matrix = new_matrix;
	ret.size[0] = size_x;
	ret.size[1] = size_y;
	return ret;
}


rectangle rotate_matrix(rectangle rect, float a, float b, float c)
{

	int32_t around_x = rect.size[0] / 2;
	int32_t around_y = rect.size[1] / 2;
	int8_t *new_matrix = calloc(rect.size[0] * rect.size[1], sizeof(uint8_t));

	memset(new_matrix, -1, rect.size[0] * rect.size[1]);

	for (int32_t y = 0; y < rect.size[1]; y++) {
		for (int32_t x = 0; x < rect.size[0]; x++) {
			if (rect.matrix[get_position(x, y, rect.size[0])] >= 0) {

				int32_t new_x = (int) round((a / c) * (x - around_x) - (b / c) * (y - around_y) + around_x);
				int32_t new_y = rect.size[1] - (int) round((b / c) * (x - around_x) + (a / c) * (y - around_y) + around_y);

				if (new_x < rect.size[0] && new_y < rect.size[1] && new_x >= 0 && new_y >= 0) {
					new_matrix[get_position(new_x, new_y, rect.size[0])] = rect.matrix[get_position(x, y, rect.size[0])];
				}
			}
		}
	}

	rectangle ret_rect;
	ret_rect.matrix = new_matrix;
	ret_rect.size[0] = rect.size[0];
	ret_rect.size[1] = rect.size[1];
	return ret_rect;
}


void display_matrix(int8_t *m, int32_t xsize, int32_t ysize)
{
	printf("\n");
	for (int32_t y = 0; y < ysize; y++) {
		for (int32_t x = 0; x < xsize; x++) {
			if (m[get_position(x, y, xsize)] >= 0) {
				printf("%2d ", m[get_position(x, y, xsize)]);
			} else {
				printf(" . ");
			}
		}
		printf("\n");
	}
}


rectangle find_max_rectangle_in_matrix(rectangle rect)
{
	// find upper and bottom line
	int bottom = 0;
	int top = -1;

	for (int y = 0; y < rect.size[1] ; y++) {
		int val_bottom = 0;
		int full_line = 0;
		for (int x = 0; x < rect.size[0]; x++) {
			if (rect.matrix[get_position(x, y, rect.size[0])] >= 0) {
				full_line++;
			}
		}

		if (full_line == rect.size[0] && top == -1) {
			top = y;
		}

		if (full_line == rect.size[0] && y > bottom) {
			bottom = y;
		}
	}

	uint32_t size = bottom - top;
	int8_t *rect_matrix = calloc(size * rect.size[0], sizeof(int8_t));
	int line = 0;

	for (int y = top; y < bottom; y++) {
		for (int x = 0; x < rect.size[0]; x++) {
			rect_matrix[get_position(x, line, rect.size[0])] = rect.matrix[get_position(x, y, rect.size[0])];
		}
		line++;
	}

	rectangle r;
	r.matrix = rect_matrix;
	r.size[0] = rect.size[0];
	r.size[1] = size;
	return r;
}




void convert_pixel(pixel *source, pixel *target)
{
	target->r = max(min(source->r, 255), 0);
	target->g = max(min(source->g, 255), 0);
	target->b = max(min(source->b, 255), 0);
}

float color_delta_f_ccir601(unsigned char c1[3], unsigned char c2[3])
{
	int dr = abs((c1[0]) - (c2[0]));
	int dg = abs((c1[1]) - (c2[1]));
	int db = abs((c1[2]) - (c2[2]));

	return sqrt(30 * (dr * dr) + 59 * (dg * dg) + 11 * (db * db));
}

float color_delta_f(unsigned char c1[3], unsigned char c2[3])
{
	int dr = abs((c1[0]) - (c2[0]));
	int dg = abs((c1[1]) - (c2[1]));
	int db = abs((c1[2]) - (c2[2]));

	return sqrt((dr * dr) + (dg * dg) + (db * db));
}

void find_closest_color_rgb(pixel *pixel, palette *palette, rgb_color *color)
{

	float d_plt;
	unsigned char c_current[3];
	float diff = FLT_MAX;
	unsigned char plt = 0;

	c_current[0] = pixel->r;
	c_current[1] = pixel->g;
	c_current[2] = pixel->b;

	for (int i = 0; i < palette->size; i++) {
		d_plt = color_delta_f_ccir601(palette->colors[i], c_current);
		if (d_plt < diff) {
			plt = i;
			diff = d_plt;
		}
	}

	(*color)[0] = palette->colors[plt][0];
	(*color)[1] = palette->colors[plt][1];
	(*color)[2] = palette->colors[plt][2];
}


short linear_space(int8_t x)
{
	float y;
	float fx = (float) x / 255.0;
	if (fx <= 0.04045) {
		y = fx / 12.92;
	} else {
		y = pow(((fx + 0.055) / 1.055), 2.4);
	}
	return (short) round(y * 255.0);
}


unsigned char *ordered_dither(unsigned char *source, int width, int height, int channels, 
palette *palette, int8_t *matrix, int32_t *matrix_size)
{
	pixel d, d2;

	short index_color;
	float map_value;
	uint8_t value;

	unsigned char *target = calloc(width * height * channels, sizeof(char));
	
	printf("buffer size: %d\n", width * height * channels);

	if (target == NULL) {
		fprintf(stderr, "ordered_dither - not enough memory to create image");
		return NULL;
	}

	pixel p;
	rgb_color c;
	int ptr_x = 0;

	for (short y = 0; y < height; y++) {
		ptr_x = 0;
		for (short x = 0; x < width * 3; x += channels) {

			p.r = (short)source[y * width * 3 + x];
			p.g = (short)source[y * width * 3 + x + 1];
			p.b = (short)source[y * width * 3 + x + 2];
			
			map_value = matrix[matrix_size[0] * y + ptr_x];
			value =  (uint8_t) round(map_value * 255 / matrix_max_value);
			
			d2.r = p.r + value - 127;
			d2.g = p.g + value - 127;
			d2.b = p.b + value - 127;

//			TODO test linear space
//			d2.r = linear_space(p.r) + value - 127;
//			d2.g = linear_space(p.g) + value - 127;
//			d2.b = linear_space(p.b) + value - 127;

			convert_pixel(&d2, &d);
			find_closest_color_rgb(&d, palette, &c);

			target[y * width * 3 + x] = c[0];
			target[y * width * 3 + x + 1] = c[1];
			target[y * width * 3 + x + 2] = c[2];

			ptr_x++;
		}
	}

	return target;
}



rgb get_rgb(unsigned char *source, int width, int height, int channels)
{
	uint8_t *rcomps = calloc(width * height, sizeof(char));
	uint8_t *gcomps = calloc(width * height, sizeof(char));
	uint8_t *bcomps = calloc(width * height, sizeof(char));
	pixel p;
	int ptr = 0;
	for (short y = 0; y < height; y++) {
		for (short x = 0; x < width * 3; x += channels) {
			p.r = (short)source[y * width * 3 + x];
			p.g = (short)source[y * width * 3 + x + 1];
			p.b = (short)source[y * width * 3 + x + 2];
			
			rcomps[ptr] = (unsigned char) round(255.0 * R(p.r, p.g, p.b));
			gcomps[ptr] = (unsigned char) round(255.0 * G(p.r, p.g, p.b));
			bcomps[ptr] = (unsigned char) round(255.0 * B(p.r, p.g, p.b));
			ptr++;
		}
	}
	
	rgb comps;
	comps.r = rcomps;
	comps.g = gcomps;
	comps.b = bcomps;
	
	return comps;
}

uint8_t *get_comp_as_image(unsigned char *source, int width, int height, int channels)
{
	uint8_t *target = calloc(width * height * channels, sizeof(char));
	int ptr = 0;
	for (short y = 0; y < height; y++) {
		for (short x = 0; x < width * 3; x += channels) {
			target[y * width * 3 + x] = source[ptr];
			target[y * width * 3 + x + 1] = source[ptr];
			target[y * width * 3 + x + 2] = source[ptr];
			ptr++;
		}
	}
	
	return target;
}

void change_white_to_color(uint8_t *img, int width, int height, int channels, rgb_color color)
{
	for (short y = 0; y < height; y++) {
		for (short x = 0; x < width * 3; x += channels) {
			if (img[y * width * 3 + x] == 255 && img[y * width * 3 + x + 1] == 255 && img[y * width * 3 + x + 2] == 255) {
				img[y * width * 3 + x] = color[0];
				img[y * width * 3 + x + 1] = color[1];
				img[y * width * 3 + x + 2] = color[2];
			}
		}
	}
}

rgb dither_components(rgb *components, int width, int height, int channels, int8_t *matrix, int32_t *matrix_size)
{
	palette bw;
	bw.size = 2;
	bw.colors[0][0] = 0;
	bw.colors[0][1] = 0;
	bw.colors[0][2] = 0;
	bw.colors[1][0] = 255;
	bw.colors[1][1] = 255;
	bw.colors[1][2] = 255;
	
	// red
	uint8_t *rcomp_img = get_comp_as_image(components->r, width, height, channels);
	unsigned char *r_result = ordered_dither(rcomp_img, width, height, channels, &bw, matrix, matrix_size);
	rgb_color red = {255, 0, 0};
	change_white_to_color(r_result, width, height, channels, red);
	free(rcomp_img);
	
	// green
	uint8_t *gcomp_img = get_comp_as_image(components->g, width, height, channels);
	unsigned char *g_result = ordered_dither(gcomp_img, width, height, channels, &bw, matrix, matrix_size);
	rgb_color green = {0, 255, 0};
	change_white_to_color(g_result, width, height, channels, green);
	free(gcomp_img);
	
	// blue
	uint8_t *bcomp_img = get_comp_as_image(components->b, width, height, channels);
	unsigned char *b_result = ordered_dither(bcomp_img, width, height, channels, &bw, matrix, matrix_size);
	rgb_color blue = {0, 0, 255};
	change_white_to_color(b_result, width, height, channels, blue);
	free(bcomp_img);
	
	rgb dithered_comps;
	dithered_comps.r = r_result;
	dithered_comps.g = g_result;
	dithered_comps.b = b_result;
	
	return dithered_comps;
}


uint8_t *add_components(rgb components, int width, int height, int channels)
{
	uint8_t *target = calloc(width * height * channels, sizeof(char));
	int ptr = 0;
	for (short y = 0; y < height; y++) {
		for (short x = 0; x < width * 3; x += channels) {
			target[y * width * 3 + x] = (components.r[y * width * 3 + x] | components.g[y * width * 3 + x] | components.b[y * width * 3 + x]);
			target[y * width * 3 + x + 1] = (components.r[y * width * 3 + x + 1] | components.g[y * width * 3 + x + 1] | components.b[y * width * 3 + x + 1]);
			target[y * width * 3 + x + 2] = (components.r[y * width * 3 + x + 2] | components.g[y * width * 3 + x + 2] | components.b[y * width * 3 + x + 2]);
		}
	}
	return target;
}





int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Image filename as argument...\n");
		return 0;
	}

	printf("Generating a big matrix from \n");
	display_matrix(matrix, 4, 4);
	printf("\n");

	
	rectangle repeat_matrix = create_matrix(matrix, repeat_matrix_size[0], repeat_matrix_size[1]);
	rectangle rotated_matrix = rotate_matrix(repeat_matrix, _A, _B, _C);
	rectangle the_big_matrix = find_max_rectangle_in_matrix(rotated_matrix);

	printf("Loading image\n");
	int width, height, channels;
	unsigned char *img = stbi_load(argv[1], &width, &height, &channels, 0);

	if (img == NULL) {
		printf("Error in loading the image\n");
		exit(1);
	}
	printf("Loaded image with a width of %dpx, a height of %dpx and %d channels\n", width, height, channels);
	
	if (width > the_big_matrix.size[0] || height > the_big_matrix.size[1]) {
		printf("Picture size is too high - use a %dx%d max picture size\n", the_big_matrix.size[0], the_big_matrix.size[1]);
		exit(1);
	}
	
	// CMYK test
	rgb comps = get_rgb(img, width, height, channels);
	uint8_t *rcomp_img = get_comp_as_image(comps.r, width, height, channels);
	uint8_t *gcomp_img = get_comp_as_image(comps.g, width, height, channels);
	uint8_t *bcomp_img = get_comp_as_image(comps.b, width, height, channels);
	stbi_write_png("images/r.png", width, height, channels, rcomp_img, width * channels);
	stbi_write_png("images/g.png", width, height, channels, gcomp_img, width * channels);
	stbi_write_png("images/b.png", width, height, channels, bcomp_img, width * channels);
	rgb dithered_comps = dither_components(&comps, width, height, channels, the_big_matrix.matrix, the_big_matrix.size);
	stbi_write_png("images/rd.png", width, height, channels, dithered_comps.r, width * channels);
	stbi_write_png("images/gd.png", width, height, channels, dithered_comps.g, width * channels);
	stbi_write_png("images/bd.png", width, height, channels, dithered_comps.b, width * channels);
	uint8_t *rgb_img = add_components(dithered_comps, width, height, channels);
	
	char new_name[1024] = {0};

	size_t length;
	const char *base;
	cwk_path_get_basename(argv[1], &base, &length);
	
	const char *path = argv[1];
	cwk_path_get_dirname(path, &length);
	
	strncpy(new_name, path, length);
	strcat(new_name, "_");
	strcat(new_name, base);
	stbi_write_png(new_name, width, height, channels, rgb_img, width * channels);
	
	free(comps.r);
	free(comps.g);
	free(comps.b);
	free(dithered_comps.r);
	free(dithered_comps.g);
	free(dithered_comps.b);
	free(rgb_img);

	free(repeat_matrix.matrix);
	free(rotated_matrix.matrix);
	free(the_big_matrix.matrix);
	return 0;
}
