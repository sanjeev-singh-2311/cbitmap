#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define PIXALS_X 64
#define PIXALS_Y 64
#define BITS_PER_PIXAL 24
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

typedef struct {
	SIGNED_BYTE_8 x;
	SIGNED_BYTE_8 y;
} Point;

typedef struct {
	uint64_t size;
	Point* p_arr;
} DynPointsArr;

typedef struct {
	Point p1;
	Point p2;
	Point p3;
	BYTE_4 col;
} Triangle;

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

void swap(Point* x, Point* y) {
	Point temp = *x;
	*x = *y;
	*y = temp;
}

void plot_line_low(Point p0, Point p1, BYTE_4 col, DEFINE_GRID(arr), DynPointsArr* shadables);
void plot_line_high(Point p0, Point p1, BYTE_4 col, DEFINE_GRID(arr), DynPointsArr* shadables);

void plot_line(Point p0, Point p1, BYTE_4 col, DEFINE_GRID(arr), DynPointsArr* shadables) {
	SIGNED_BYTE_8 x0 = p0.x, y0 = p0.y;
	SIGNED_BYTE_8 x1 = p1.x, y1 = p1.y;
	if (labs(y1 - y0) < labs(x1 - x0)) {
		if (x0 > x1) {
			plot_line_low(p1, p0, col, arr, shadables);
		} else {
			plot_line_low(p0, p1, col, arr, shadables);
		}
	} else {
		if (y0 > y1) {
			plot_line_high(p1, p0, col, arr, shadables);
		} else {
			plot_line_high(p0, p1, col, arr, shadables);
		}
	}
	return;
}

void plot_line_low(Point p0, Point p1, BYTE_4 col, DEFINE_GRID(arr), DynPointsArr* shadables) {
	SIGNED_BYTE_8 x0 = p0.x, y0 = p0.y;
	SIGNED_BYTE_8 x1 = p1.x, y1 = p1.y;

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
		if (shadables != NULL)
			shadables->p_arr[shadables->size++] = (Point){x, y};
		if (D > 0) {
			y += yi;
			D += (dy - dx) << 1;
		} else {
			D += dy << 1;
		}
	}
	return;
}

void plot_line_high(Point p0, Point p1, BYTE_4 col, DEFINE_GRID(arr), DynPointsArr* shadables) {
	SIGNED_BYTE_8 x0 = p0.x, y0 = p0.y;
	SIGNED_BYTE_8 x1 = p1.x, y1 = p1.y;

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
		if (shadables != NULL)
			shadables->p_arr[shadables->size++] = (Point){x, y};
		if (D > 0) {
			x += xi;
			D += (dx - dy) << 1;
		} else {
			D += dx << 1;
		}
	}
	return;
}

void draw_line(Point* p_arr, BYTE_8 n, BYTE_4 col, DEFINE_GRID(arr), DynPointsArr* shadables) {
	for (BYTE_8 i = 0; i < n; i++) {
		Point p0 = p_arr[i];
		Point p1 = p_arr[(i + 1) % n];
		plot_line(p0, p1, col, arr, shadables);
	}
}

void sort(DynPointsArr* shadables) {
	for (uint64_t i = 0; i < shadables->size; i++) {
		for (uint64_t j = i + 1; j < shadables->size; j++) {
			int condition = (shadables->p_arr[i].x > shadables->p_arr[j].x) 
				|| (
				(shadables->p_arr[i].x == shadables->p_arr[j].x) && (shadables->p_arr[i].y > shadables->p_arr[j].y)
			);

			if (condition) {
				Point temp = shadables->p_arr[i];
				shadables->p_arr[i] = shadables->p_arr[j];
				shadables->p_arr[j] = temp;
			}
		}
	}
}

void shade_triangle(DynPointsArr* shadables, BYTE_4 col, DEFINE_GRID(arr)) {
	sort(shadables);

	uint64_t l = 0, r = 1;

	while (l < shadables->size) {
		while (r < shadables->size && shadables->p_arr[l].x == shadables->p_arr[r].x)
			r++;
		if (r != shadables->size - 1) r--;

		plot_line(shadables->p_arr[l], shadables->p_arr[r], col, arr, NULL);

		l = r + 1;
		r = l + 1;
	}
}

void draw_triangle(Triangle t, DEFINE_GRID(arr), DynPointsArr* shadables) {
	Point p_arr[] = {t.p1, t.p2, t.p3};
	draw_line(p_arr, 3, t.col, arr, shadables);
	shade_triangle(shadables, t.col, arr);
}

void draw_polygon(Point* p_arr, BYTE_8 n, BYTE_4 col, DEFINE_GRID(arr)) {
	DynPointsArr* shadables = (DynPointsArr*)malloc(sizeof(DynPointsArr));
	shadables->p_arr = malloc(sizeof(Point) * 4 * PIXALS_X);
	for (BYTE_8 i = 0; i + 2 < n; i += 3) {
		shadables->size = 0;
		draw_triangle(
			(Triangle) {
				p_arr[i],
				p_arr[i + 1],
				p_arr[i + 2],
				col
			},
			arr,
			shadables
		);
		for (uint64_t i = 0; i < shadables->size; i++) {
			printf("(%ld, %ld)  ", shadables->p_arr[i].x, shadables->p_arr[i].y);
		}
		printf("\n");
	}
	free(shadables->p_arr);
	shadables->p_arr = NULL;
	free(shadables);
	shadables = NULL;
}

void draw_polygon_slice(Point* p_arr, BYTE_8 n, BYTE_4 col, DEFINE_GRID(arr)) {
	DynPointsArr* shadables = (DynPointsArr*)malloc(sizeof(DynPointsArr));
	shadables->p_arr = malloc(sizeof(Point) * 4 * PIXALS_X);
	for (BYTE_8 i = 0; i + 2 < n; i++) {
		shadables->size = 0;
		draw_triangle(
			(Triangle) {
				p_arr[i],
				p_arr[i + 1],
				p_arr[i + 2],
				col
			},
			arr,
			shadables
		);
		for (uint64_t i = 0; i < shadables->size; i++) {
			printf("(%ld, %ld)  ", shadables->p_arr[i].x, shadables->p_arr[i].y);
		}
		printf("\n");
	}
	free(shadables->p_arr);
	shadables->p_arr = NULL;
	free(shadables);
	shadables = NULL;
}

void write_pixels(FILE* f) {
	DEFINE_GRID(pixel_array) = {0};
	BYTE_8 row_padding = (4 - (PIXALS_X * (BITS_PER_PIXAL >> 3)) % 4) % 4;
	BYTE_4 padding = 0;

	Point polygon[] = {
		{10, 10}, {10, 20}, {20, 10},
		{10, 20}, {20, 10}, {20, 20},
		{10, 20}, {20, 20}, {15, 30},
		{15, 30}, {20, 20}, {25, 30},
		{20, 10}, {25, 15}, {25, 30},
		{20, 20}, {20, 10}, {25, 30}
	};

	draw_polygon(polygon, 18, 0xFFFFFA, pixel_array);

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
