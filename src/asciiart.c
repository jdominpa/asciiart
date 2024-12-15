#include <assert.h>
#include <stdint.h>
#include <stdio.h>
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

static_assert(ASCII_CHAR_COUNT == 10, "Amount of ascii characters has changed");
static_assert(ASCII_CHAR_SIZE == 8, "Size of ascii characters has changed");
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

Ascii_char grayscale_to_ascii(uint8_t gray_value)
{
    // 0..255 (grayscale value) -> 0..9 (Ascii_char index)
    return gray_value * (ASCII_CHAR_COUNT-1) / 255;
}

void convert_pixels_to_ascii(uint8_t *pixels, uint8_t *downscaled_pixels, size_t w, size_t h)
{
    for (size_t y = 0; y < h; y += ASCII_CHAR_SIZE) {
        for (size_t x = 0; x < w; x += ASCII_CHAR_SIZE) {
            size_t i = y*w + x;
            Ascii_char ascii_char = grayscale_to_ascii(downscaled_pixels[i]);
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

int main(int argc, char **argv)
{
    const char *program_name = shift(argv, argc);
    if (argc <= 0) {
        fprintf(stderr, "Usage: %s <input_image_path> <output_image_path>\n", program_name);
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
    uint8_t downscaled_pixels[width*height];
    memcpy(downscaled_pixels, pixels, width*height);
    downscale_img_res(downscaled_pixels, width, height, 1, ASCII_CHAR_SIZE);
    convert_pixels_to_ascii(pixels, downscaled_pixels, width, height);

    if (!stbi_write_png(output_path, width, height, 1, pixels, width)) {
        fprintf(stderr, "ERROR: Could not save output image %s\n", output_path);
        return 1;
    }

    return 0;
}
