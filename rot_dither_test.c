#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <float.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"


#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))

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

typedef struct _histo_cell {
	int32_t start, end, size;
} histo_cell;


int32_t repeat_matrix_size[] = { 20, 20 };
int32_t wrapping_matrix_size[] = { 40, 40 };
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


void display_matrix_as_dot(int8_t *m, int32_t xsize, int32_t ysize)
{
	printf("\n");
	for (int32_t y = 0; y < ysize; y++) {
		for (int32_t x = 0; x < xsize; x++) {
			if (m[get_position(x, y, xsize)] >= 0) {
				printf("■");
			} else {
				printf(" ");
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




int main(int argc, char *argv[])
{

	printf("Generating a test matrix from \n");
	display_matrix(matrix, 4, 4);
	printf("\n");
	
	rectangle repeat_matrix = create_matrix(matrix, repeat_matrix_size[0], repeat_matrix_size[1]);
	display_matrix(repeat_matrix.matrix, repeat_matrix.size[0], repeat_matrix.size[1]);
	
	printf("\nCentering matrix a an wrapping one\n");
	rectangle centered_matrix = center_matrix(repeat_matrix, wrapping_matrix_size[0], wrapping_matrix_size[1]);
	display_matrix(centered_matrix.matrix, centered_matrix.size[0], centered_matrix.size[1]);
	printf("\n");
	
	printf("Making a discrete rotation\n");
	rectangle rotated_matrix = rotate_matrix(centered_matrix, _A, _B, _C);
	display_matrix(rotated_matrix.matrix, rotated_matrix.size[0], rotated_matrix.size[1]);
	display_matrix_as_dot(rotated_matrix.matrix, rotated_matrix.size[0], rotated_matrix.size[1]);

	free(repeat_matrix.matrix);
	free(centered_matrix.matrix);
	free(rotated_matrix.matrix);
	return 0;
}