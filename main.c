#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define PIXALS_X 128
#define PIXALS_Y 128
#define BITS_PER_PIXAL 32
#define FILE_SIZE (14 + 40 + PIXALS_X * PIXALS_Y * BITS_PER_PIXAL / 8)

#define DEFINE_GRID(arr) BYTE_4 arr[PIXALS_Y][PIXALS_X]

typedef uint8_t BYTE_1;
typedef uint16_t BYTE_2;
typedef uint32_t BYTE_4;
typedef uint64_t BYTE_8;

typedef int8_t SIGNED_BYTE_1;
typedef int16_t SIGNED_BYTE_2;
typedef int32_t SIGNED_BYTE_4;
typedef int64_t SIGNED_BYTE_8;

#define POINTS_SIZE 8
// static uint64_t points[][3] = {{10, 10, 0xF10010}, {10, 40, 0x01FF11}, {40, 40, 0x1100F1}, {40, 10, 0xFF0FF0}};
static uint64_t points[][3] = {
    {22, 62, 0x4F7B29}, {118, 42, 0xF1A4D2}, {110, 110, 0x8A9F77}, {25, 118, 0xC5308E},
    {39, 25, 0xD6A4E7}, {104, 12, 0x8E3D1F}, {111, 92, 0x1C74B3}, {28, 100, 0x3F57A9}
};

#define PANIC(s) { \
    printf(s); \
    exit(-1); \
}

BYTE_8 little_endian(BYTE_8 num) {
	BYTE_8 res = 0;
	for (size_t i = 0; i < 8; ++i) {
		res = (res << 8) | (num & 0xFF);
		num >>= 8;
	}
	return res;
}

void write_header(FILE* f) {
	// 14 bytes header
	BYTE_1 header[14] = {0};
	
	// 2 Bytes -> BM
	header[0] |= 'B';
	header[1] |= 'M';
	// 4 Bytes size of file
	header[2] |= FILE_SIZE & 0xFF;
	header[3] |= (FILE_SIZE >> 8) & 0xFF;
	header[4] |= (FILE_SIZE >> 16) & 0xFF;
	header[5] |= (FILE_SIZE >> 24) & 0xFF;
	// 2 Bytes reserved, keep it 0 -> 6 and 7
	// 2 Bytes reserved, keep it 0 -> 8 and 9
	// 4 Bytes offset of where the pixal data start.. 14 bytes header and 40 Bytes DIB gives it 54th byte or 0x36
	header[10] |= 54 & 0xFF;
	header[11] |= (54 >> 8) & 0xFF;
	header[12] |= (54 >> 16) & 0xFF;
	header[13] |= (54 >> 24) & 0xFF;
	
	// write header to file
	fwrite(header, 14, 1, f);
}

void write_dib(FILE* f) {
	// 40 Bytes DIB
	BYTE_1 dib[40] = {0};

	// 4 Bytes size of header -> 40
	dib[0] |= 40 & 0xFF;
	dib[1] |= (40 >> 8) & 0xFF;
	dib[2] |= (40 >> 16) & 0xFF;
	dib[3] |= (40 >> 24) & 0xFF;
	// 4 Bytes bitmap width
	dib[4] |= PIXALS_X & 0xFF;
	dib[5] |= (PIXALS_X >> 8) & 0xFF;
	dib[6] |= (PIXALS_X >> 16) & 0xFF;
	dib[7] |= (PIXALS_X >> 24) & 0xFF;
	// 4 Bytes bitmap height
	dib[8] |= PIXALS_Y & 0xFF;
	dib[9] |= (PIXALS_Y >> 8) & 0xFF;
	dib[10] |= (PIXALS_Y >> 16) & 0xFF;
	dib[11] |= (PIXALS_Y >> 24) & 0xFF;
	// 2 bytes no of colour pane
	dib[12] |= 1 & 0xFF;
	dib[13] |= (1 >> 8) & 0xFF;
	// 2 bytes bits per pixal
	dib[14] |= BITS_PER_PIXAL & 0xFF;
	dib[15] |= (BITS_PER_PIXAL >> 8) & 0xFF;
	// 4 Bytes compression method uses -> none = 0
	// 4 Bytes image size, dummy 0 for BI_RGB since no compression
	dib[23] = 0;

	// 4 Byte Horizontal and Vertical resolution
	dib[24] |= 1920 & 0xFF;
	dib[25] |= (1920 >> 8) & 0xFF;
	dib[26] |= (1920 >> 16) & 0xFF;
	dib[27] |= (1920 >> 24) & 0xFF;
	dib[28] |= 1080 & 0xFF;
	dib[29] |= (1080 >> 8) & 0xFF;
	dib[30] |= (1080 >> 16) & 0xFF;
	dib[31] |= (1080 >> 24) & 0xFF;

	// 4 byte number of colours in palette -> 0 for 2^n and 4 Bytes for important colours -> 0

	fwrite(dib, 40, 1, f);
}

void swap(int* x, int* y) {
	int temp = *x;
	*x = *y;
	*y = temp;
}

void plot_line_low(SIGNED_BYTE_4 x0, SIGNED_BYTE_4 x1, SIGNED_BYTE_4 y0, SIGNED_BYTE_4 y1, BYTE_4 col, DEFINE_GRID(arr));
void plot_line_high(SIGNED_BYTE_4 x0, SIGNED_BYTE_4 x1, SIGNED_BYTE_4 y0, SIGNED_BYTE_4 y1, BYTE_4 col, DEFINE_GRID(arr));

void plot_line(SIGNED_BYTE_4 x0, SIGNED_BYTE_4 x1, SIGNED_BYTE_4 y0, SIGNED_BYTE_4 y1, BYTE_4 col, DEFINE_GRID(arr)) {
	if (abs(y1 - (SIGNED_BYTE_4)y0) < abs(x1 - (SIGNED_BYTE_4)x0)) {
		if (x0 > x1) {
			plot_line_low(x1, x0, y1, y0, col, arr);
		} else {
			plot_line_low(x0, x1, y0, y1, col, arr);
		}
	} else {
		if (y0 > y1) {
			plot_line_high(x1, x0, y1, y0, col, arr);
		} else {
			plot_line_high(x0, x1, y0, y1, col, arr);
		}
	}
	return;
}

void plot_line_low(SIGNED_BYTE_4 x0, SIGNED_BYTE_4 x1, SIGNED_BYTE_4 y0, SIGNED_BYTE_4 y1, BYTE_4 col, DEFINE_GRID(arr)) {
	SIGNED_BYTE_4 dx = x1 - x0;
	SIGNED_BYTE_4 dy = y1 - y0;
	SIGNED_BYTE_4 yi = 1;
	if (dy < 0) {
		yi = -1;
		dy = -dy;
	}

	SIGNED_BYTE_4 D = (dy << 1) - dx;
	BYTE_4 y = y0;

	for (SIGNED_BYTE_4 x = x0; x <= x1; x++) {
		arr[y][x] = col;
		if (D > 0) {
			y += yi;
			D += (dy - dx) << 1;
		} else {
			D += dy << 1;
		}
	}
	return;
}

void plot_line_high(SIGNED_BYTE_4 x0, SIGNED_BYTE_4 x1, SIGNED_BYTE_4 y0, SIGNED_BYTE_4 y1, BYTE_4 col, DEFINE_GRID(arr)) {
	SIGNED_BYTE_4 dx = x1 - x0;
	SIGNED_BYTE_4 dy = y1 - y0;
	SIGNED_BYTE_4 xi = 1;
	if (dx < 0) {
		xi = -1;
		dx = -dx;
	}

	SIGNED_BYTE_4 D = (dx << 1) - dy;
	BYTE_4 x = x0;

	for (SIGNED_BYTE_4 y = y0; y <= y1; y++) {
		arr[y][x] = col;
		if (D > 0) {
			x += xi;
			D += (dx - dy) << 1;
		} else {
			D += dx << 1;
		}
	}
	return;
}

void draw_line(DEFINE_GRID(arr)) {
	
	for (size_t i = 0; i < POINTS_SIZE; i++) {
		SIGNED_BYTE_4 x0 = points[i][0];
		SIGNED_BYTE_4 x1 = points[(i + 1) % POINTS_SIZE][0];
		SIGNED_BYTE_4 y0 = points[i][1];
		SIGNED_BYTE_4 y1 = points[(i + 1) % POINTS_SIZE][1];
		BYTE_4 col = points[i][2];

		plot_line(x0, x1, y0, y1, col, arr);
	}

}

void write_pixels(FILE* f) {
	DEFINE_GRID(pixel_array) = {0};
  	BYTE_8 row_padding = (4 - (PIXALS_X * (BITS_PER_PIXAL / 8)) % 4) % 4;
	BYTE_4 padding = 0;
	
	for (uint64_t j = 0; j < PIXALS_Y; j++) {
		for (uint64_t i = 0; i < PIXALS_X; i++) {
			pixel_array[j][i] = 0xFFFFFFFF;
		}
	}
	
	draw_line(pixel_array);

	for (uint64_t j = 0; j < PIXALS_Y; j++) {
		for (uint64_t i = 0; i < PIXALS_X; i++) {
			fwrite(&pixel_array[j][i], 1, BITS_PER_PIXAL / 8, f);
		}
		if (row_padding) {
			fwrite(&padding, 1, row_padding, f);
		}
	}

}

int main(void) {
    FILE *out = fopen("output.bmp", "wb");

    write_header(out);
	write_dib(out);
	write_pixels(out);
    fclose(out);

    return 0;
}
