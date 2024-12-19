#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize2.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define shift(xs, xs_sz) (assert((xs_sz) > 0), (xs_sz)--, *(xs)++)

#define PIXEL_LAYOUT STBIR_1CHANNEL

void downscale_img_res(uint8_t *pixels, size_t w, size_t h, size_t comp, float downscale_factor)
{
    size_t rescaled_w = w/downscale_factor;
    size_t rescaled_h = h/downscale_factor;
    uint8_t *aux = stbir_resize_uint8_srgb(pixels, w, h, w*comp,
                                                 NULL, rescaled_w, rescaled_h, rescaled_w*comp,
                                                 PIXEL_LAYOUT);
    stbir_resize_uint8_srgb(aux, rescaled_w, rescaled_h, rescaled_w*comp,
                            pixels, w, h, w*comp,
                            PIXEL_LAYOUT);
    free(aux);
}

void sobel_filter(uint8_t *pixels, double *edge_angles, size_t w, size_t h)
{
    const int ker[3][3] = {{-1, 0, 1},
                           {-2, 0, 2},
                           {-1, 0, 1}};
    uint8_t *unfiltered_pixels = malloc(w*h*sizeof(uint8_t));
    if (!unfiltered_pixels) {
        fprintf(stderr, "ERROR: Could not allocate memory\n");
        exit(1);
    }
    memcpy(unfiltered_pixels, pixels, w*h*sizeof(uint8_t));

    for (size_t y = 1; y < h-1; y++) {
        for (size_t x = 1; x < w-1; x++) {
            int grad_x = 0;
            int grad_y = 0;
            for (size_t i = 0; i < 3; i++) {
                for (size_t j = 0; j < 3; j++) {
                    size_t xn = x + i - 1;
                    size_t yn = y + j - 1;
                    size_t index = yn*w + xn;
                    grad_x += unfiltered_pixels[index] * ker[i][j];
                    grad_y += unfiltered_pixels[index] * ker[j][i];
                }
            }
            pixels[w*y + x] = (uint8_t) sqrt(grad_x*grad_x + grad_y*grad_y);
            edge_angles[w*y + x] = atan2(grad_y, grad_x);
        }
    }

    free(unfiltered_pixels);
}

void detect_img_edges(uint8_t *pixels, double *edge_angles, size_t w, size_t h, uint8_t edge_threshold)
{
    sobel_filter(pixels, edge_angles, w, h);
    for (size_t i = 0; i < w*h; i++) {
        pixels[i] = (pixels[i] >= edge_threshold) ? 1 : 0;
    }
}

#define ASCII_CHAR_SIZE 8

typedef enum {
    SPACE,
    DOT,
    COLON,
    LCASE_C,
    LCASE_O,
    UPCASE_P,
    UPCASE_O,
    QUESTION_MARK,
    PERCENT,
    SQUARE,
    ASCII_CHAR_COUNT,
} Ascii_char;

static_assert(ASCII_CHAR_COUNT == 10, "Amount of ASCII characters has changed");
static_assert(ASCII_CHAR_SIZE == 8, "Size of ASCII characters has changed");
const uint8_t ascii_char_pixel_map[ASCII_CHAR_COUNT][ASCII_CHAR_SIZE] = {
    [SPACE]         = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    [DOT]           = {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x00},
    [COLON]         = {0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x00},
    [LCASE_C]       = {0x00, 0x00, 0x1E, 0x33, 0x03, 0x33, 0x1E, 0x00},
    [LCASE_O]       = {0x00, 0x00, 0x1E, 0x33, 0x33, 0x33, 0x1E, 0x00},
    [UPCASE_P]      = {0x3F, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x0F, 0x00},
    [UPCASE_O]      = {0x1C, 0x36, 0x63, 0x63, 0x63, 0x36, 0x1C, 0x00},
    [QUESTION_MARK] = {0x1E, 0x33, 0x30, 0x18, 0x0C, 0x00, 0x0C, 0x00},
    [PERCENT]       = {0x00, 0x63, 0x33, 0x18, 0x0C, 0x66, 0x63, 0x00},
    [SQUARE]        = {0x00, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x7E, 0x00},
};

Ascii_char grayvalue_to_ascii_char(uint8_t gray_value)
{
    // 0..255 (grayscale value) -> 0..9 (Ascii_char index)
    return gray_value * (ASCII_CHAR_COUNT-1) / 255;
}

typedef enum {
    UNDERSCORE,
    SLASH,
    PIPE,
    BACKSLASH,
    ASCII_EDGE_CHAR_COUNT,
} Ascii_edge_char;

static_assert(ASCII_EDGE_CHAR_COUNT == 4, "Amount of ASCII edge characters has changed");
static_assert(ASCII_CHAR_SIZE == 8, "Size of ASCII characters has changed");
const uint8_t ascii_edge_char_pixel_map[ASCII_EDGE_CHAR_COUNT][ASCII_CHAR_SIZE] = {
    [UNDERSCORE] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF},
    [SLASH]      = {0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x01, 0x00},
    [PIPE]       = {0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00},
    [BACKSLASH]  = {0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x40, 0x00},
};

Ascii_edge_char angle_to_ascii_edge_char(double theta)
{
    theta /= 4*atan(1); // PI = 4*atan(1)
    double abs_theta = fabs(theta);
    if (0.0f <= abs_theta && abs_theta < 0.05f) return UNDERSCORE;
    else if (0.05f <= abs_theta && abs_theta < 0.45f) return (theta > 0.0f) ? SLASH : BACKSLASH;
    else if (0.45f <= abs_theta && abs_theta < 0.55f) return PIPE;
    else if (0.55f <= abs_theta && abs_theta < 0.9f) return (theta > 0.0f) ? BACKSLASH : SLASH;
    else return UNDERSCORE;
}

void convert_img_to_ascii(uint8_t *pixels, size_t w, size_t h, uint8_t edge_threshold)
{
    uint8_t *downscaled_pixels = malloc(w*h*sizeof(uint8_t));
    uint8_t *edge_pixels = malloc(w*h*sizeof(uint8_t));
    double *edge_angles = malloc(w*h*sizeof(double));
    if (!downscaled_pixels || !edge_pixels || !edge_angles) {
        fprintf(stderr, "ERROR: Could not allocate memory\n");
        exit(1);
    }
    memcpy(downscaled_pixels, pixels, w*h*sizeof(uint8_t));

    downscale_img_res(downscaled_pixels, w, h, 1, ASCII_CHAR_SIZE);
    memcpy(edge_pixels, downscaled_pixels, w*h*sizeof(uint8_t));
    detect_img_edges(edge_pixels, edge_angles, w, h, edge_threshold);

    for (size_t y = 0; y < h; y += ASCII_CHAR_SIZE) {
        for (size_t x = 0; x < w; x += ASCII_CHAR_SIZE) {
            size_t i = w*y + x;
            if (edge_pixels[i]) {
                Ascii_edge_char edge_char = angle_to_ascii_edge_char(edge_angles[i]);
                for (size_t y_offset = 0; y_offset < ASCII_CHAR_SIZE; y_offset++) {
                    for (size_t x_offset = 0; x_offset < ASCII_CHAR_SIZE; x_offset++) {
                        if (x+x_offset < w && y+y_offset < h) {
                            pixels[i + x_offset + w*y_offset] =
                                255 * ((ascii_edge_char_pixel_map[edge_char][y_offset] >> x_offset) & 1);
                        }
                    }
                }
            } else {
                Ascii_char ascii_char = grayvalue_to_ascii_char(downscaled_pixels[i]);
                for (size_t y_offset = 0; y_offset < ASCII_CHAR_SIZE; y_offset++) {
                    for (size_t x_offset = 0; x_offset < ASCII_CHAR_SIZE; x_offset++) {
                        if (x+x_offset < w && y+y_offset < h) {
                            pixels[i + x_offset + w*y_offset] =
                                255 * ((ascii_char_pixel_map[ascii_char][y_offset] >> x_offset) & 1);
                        }
                    }
                }
            }
        }
    }

    free(downscaled_pixels);
    free(edge_pixels);
    free(edge_angles);
}

bool flag_int(int *argc, char ***argv, int *value)
{
    const char *flag = shift(*argv, *argc);
    if ((*argc) <= 0) {
        fprintf(stderr, "ERROR: No argument provided for %s\n", flag);
        return false;
    }
    *value = atoi(shift(*argv, *argc));
    return true;
}

int main(int argc, char **argv)
{
    const char *program_name = shift(argv, argc);
    uint8_t edge_threshold = 64;

    while (argc > 0) {
        const char *flag = argv[0];
        if (strcmp(flag, "-edge_threshold") == 0) {
            if (!flag_int(&argc, &argv, (int *) &edge_threshold)) return 1;
        } else {
            break;
        }
    }

    if (argc <= 0) {
        fprintf(stderr, "Usage: %s [options] <input_image_path> <output_image_path>\n", program_name);
        fprintf(stderr, "ERROR: No input image provided\n");
        return 1;
    }
    const char *input_path = shift(argv, argc);
    if (argc <= 0) {
        fprintf(stderr, "ERROR: No ouput image path provided\n");
        return 1;
    }
    const char *output_path = shift(argv, argc);

    size_t width, height;
    uint8_t *pixels = stbi_load(input_path, (int *)&width, (int *)&height, NULL, 1);
    if (!pixels) {
        fprintf(stderr, "ERROR: Could not load input image %s\n", input_path);
        return 1;
    }
    convert_img_to_ascii(pixels, width, height, edge_threshold);

    if (!stbi_write_png(output_path, width, height, 1, pixels, width*sizeof(uint8_t))) {
        fprintf(stderr, "ERROR: Could not save output image %s\n", output_path);
        return 1;
    }

    free(pixels);
    return 0;
}
