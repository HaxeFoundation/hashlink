#include "hl.h"
#include "gc.h"
#include "../utils.h"
#include "../suite.h"


#ifdef WIN32

#include <windows.h>
static double hires_timestamp(void) {
  LARGE_INTEGER t, f;
  QueryPerformanceCounter(&t);
  QueryPerformanceFrequency(&f);
  return (double)t.QuadPart / (double)f.QuadPart;
}

#else

#include <sys/time.h>
#include <sys/resource.h>
static double hires_timestamp(void) {
  struct timeval t;
  struct timezone tzp;
  gettimeofday(&t, &tzp);
  return t.tv_sec + t.tv_usec * 1e-6;
}

#endif

static hl_module_context mctx = {
	.functions_ptrs = NULL,
	.functions_types = NULL
};

TEST_TYPE(rgb, 3, {0}, {
	int r;
	int g;
	int b;
});
TEST_TYPE(complex, 3, {0}, {
	double i;
	double j;
});

#define MB_SIZE 25
#define MB_MAX_ITERATIONS 1000
#define MB_MAX_RAD (1 << 16)
#define MB_WIDTH (35 * MB_SIZE)
#define MB_HEIGHT (20 * MB_SIZE)

static hlt_rgb_t *createPalette(double inFraction) {
	hlt_rgb_t *ret = (hlt_rgb_t *)hl_alloc_obj(&hlt_rgb);
	ret->r = inFraction * 255;
	ret->g = (1.0 - inFraction) * 255;
	ret->b = (0.5 - abs(inFraction - 0.5)) * 2 * 255;
	return ret;
}

static hlt_complex_t *createComplex(double inI, double inJ) {
	hlt_complex_t *ret = (hlt_complex_t *)hl_alloc_obj(&hlt_complex);
	ret->i = inI;
	ret->j = inJ;
	return ret;
}

static double complexLength2(hlt_complex_t *val) {
	return val->i * val->i + val->j * val->j;
}

static hlt_complex_t *complexAdd(hlt_complex_t *val0, hlt_complex_t *val1) {
	return createComplex(val0->i + val1->i, val0->j + val1->j);
}

static hlt_complex_t *complexSquare(hlt_complex_t *val) {
	return createComplex(val->i * val->i - val->j * val->j, 2.0 * val->i * val->j);
}

BEGIN_BENCH_CASE(mandelbrot) {
	hl_alloc_init(&mctx.alloc);

	varray *palette = hl_alloc_array(&hlt_rgb, MB_MAX_ITERATIONS + 1);
	hl_add_root((void *)&palette);
	for (int i = 0; i < MB_MAX_ITERATIONS + 1; i++) {
		hl_aptr(palette, hlt_rgb_t *)[i] = createPalette((double)i / MB_MAX_ITERATIONS);
	}

	/*
	for (int i = 0; i < MB_SIZE; i++) {
		printf("%d %d %d\n", palette->data[i]->r, palette->data[i]->g, palette->data[i]->b);
	}
	*/

	varray *image = hl_alloc_array(&hlt_rgb, MB_WIDTH * MB_HEIGHT);
	hl_add_root((void *)&image);
	int outPixel = 0;
	double scale = 0.1 / MB_SIZE;

	hlt_complex_t *offset = NULL;
	hlt_complex_t *val = NULL;
	hl_add_root((void *)&offset);
	hl_add_root((void *)&val);

	for (int y = 0; y < MB_HEIGHT; y++) {
		if ((y % 10) == 0) {
			printf("%d: %d pages, %lu live objects, %lu live blocks, %lu bytes used, %lu collection cycles\n", y, gc_stats->total_pages, gc_stats->live_objects, gc_stats->live_blocks, gc_stats->total_memory, gc_stats->cycles);
		}
		for (int x = 0; x < MB_WIDTH; x++) {
			int iteration = 0;

			// "reduce_allocs"
			/*
			double offsetI = x * scale - 2.5;
			double offsetJ = y * scale - 1.0;
			double valI = 0.0;
			double valJ = 0.0;
			while (valI * valI + valJ * valJ < MB_MAX_RAD && iteration < MB_MAX_ITERATIONS) {
				double vi = valI;
				valI = valI * valI - valJ * valJ + offsetI;
				valJ = 2.0 * vi * valJ + offsetJ;
				iteration++;
			}
			*/
			// normal
			offset = createComplex(x * scale - 2.5, y * scale - 1);
			val = createComplex(0.0, 0.0);
			while (complexLength2(val) < MB_MAX_RAD && iteration < MB_MAX_ITERATIONS) {
				val = complexSquare(val);
				val = complexAdd(val, offset);
				iteration++;
			}

			hl_aptr(image, hlt_rgb_t *)[outPixel++] = hl_aptr(palette, hlt_rgb_t *)[iteration];
		}
	}

	FILE *f = fopen("mandelbrot.ppm", "w");
	fprintf(f, "P6 %d %d 255\n", MB_WIDTH, MB_HEIGHT);
	for (int i = 0; i < MB_WIDTH * MB_HEIGHT; i++) {
		hlt_rgb_t *pixel = hl_aptr(image, hlt_rgb_t *)[i];
		// printf("%d %p\n", i, pixel);
		unsigned char r = pixel->r & 0xFF;
		unsigned char g = pixel->g & 0xFF;
		unsigned char b = pixel->b & 0xFF;
		fwrite(&r, 1, 1, f);
		fwrite(&g, 1, 1, f);
		fwrite(&b, 1, 1, f);
	}
	fclose(f);
} END_BENCH_CASE
