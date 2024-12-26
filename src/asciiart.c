#include <assert.h>
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

void convert_rgba_to_grayscale(uint8_t *pixels, size_t w, size_t h, uint32_t comp)
{
    if (comp < 3) return;
    for (size_t i = 0; i < w*h; i++) {
        size_t index = comp*i;
        uint8_t gray_value = 0.2126*pixels[index] + 0.7152*pixels[index+1] + 0.0722*pixels[index+2];
        pixels[index] = pixels[index+1] = pixels[index+2] = gray_value;
    }
}

void downscale_img_res(uint8_t *pixels, size_t w, size_t h, uint32_t comp, uint32_t downscale_factor, stbir_pixel_layout pixel_layout)
{
    uint32_t rescaled_w = w/downscale_factor;
    uint32_t rescaled_h = h/downscale_factor;
    uint8_t *aux = stbir_resize_uint8_srgb(pixels, w, h, w*comp,
                                           NULL, rescaled_w, rescaled_h, rescaled_w*comp,
                                           pixel_layout);
    stbir_resize_uint8_srgb(aux, rescaled_w, rescaled_h, rescaled_w*comp,
                            pixels, w, h, w*comp,
                            pixel_layout);
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

void convert_img_to_ascii(uint8_t *pixels, size_t w, size_t h, uint32_t comp, uint32_t color, bool with_img_colors)
{
    uint8_t *downscaled_pixels = malloc(w*h*comp*sizeof(uint8_t));
    if (!downscaled_pixels) {
        fprintf(stderr, "ERROR: Could not allocate memory\n");
        exit(1);
    }

    memcpy(downscaled_pixels, pixels, w*h*comp*sizeof(uint8_t));
    convert_rgba_to_grayscale(downscaled_pixels, w, h, comp);
    downscale_img_res(downscaled_pixels, w, h, comp, ASCII_CHAR_SIZE, STBIR_RGBA);

    for (size_t y = 0; y < h; y += ASCII_CHAR_SIZE) {
        for (size_t x = 0; x < w; x += ASCII_CHAR_SIZE) {
            size_t i = comp*(w*y + x);
            Ascii_char ascii_char = grayvalue_to_ascii_char(downscaled_pixels[i]);
            for (size_t y_offset = 0; y_offset < ASCII_CHAR_SIZE; y_offset++) {
                for (size_t x_offset = 0; x_offset < ASCII_CHAR_SIZE; x_offset++) {
                    if (x+x_offset < w && y+y_offset < h) {
                        size_t index = i + comp*(x_offset + w*y_offset);
                        uint8_t r, g, b, a;
                        if (with_img_colors) {
                            r = pixels[index];
                            g = pixels[index+1];
                            b = pixels[index+2];
                            a = pixels[index+3];
                        } else {
                            r = (color >> 8*3) & 0xFF;
                            g = (color >> 8*2) & 0xFF;
                            b = (color >> 8*1) & 0xFF;
                            a = (color >> 8*0) & 0xFF;
                        }
                        pixels[index]   = r * ((ascii_char_pixel_map[ascii_char][y_offset] >> x_offset) & 1);
                        pixels[index+1] = g * ((ascii_char_pixel_map[ascii_char][y_offset] >> x_offset) & 1);
                        pixels[index+2] = b * ((ascii_char_pixel_map[ascii_char][y_offset] >> x_offset) & 1);
                        pixels[index+3] = a;
                    }
                }
            }
        }
    }

    free(downscaled_pixels);
}

void print_usage(const char *program)
{
    fprintf(stdout, "Usage: %s [options] <input_image_path> <output_image_path>\n", program);
    fprintf(stdout, "Options:\n");
    fprintf(stdout, "  --help              Display this information.\n");
    fprintf(stdout, "  --with-img-colors   Render ASCII characters with the image's original colors.\n");
    fprintf(stdout, "  --with-color        Render ASCII characters with the specified color (in RGBA format).\n");
}

uint32_t hextou32(char *hex)
{
    uint32_t res = 0;
    char c;
    while ((c = *hex++)) {
        if ('0' <= c && c <= '9') c = c - '0';
        else if ('a' <= c && c <= 'f') c = c - 'a' + 10;
        else if ('A' <= c && c <= 'F') c = c - 'A' + 10;
        else {
            fprintf(stderr, "ERROR: Invalid hexadecimal digit found: %c\n", c);
            exit(1);
        }
        res = (res << 4) | (c & 0xF);
    }
    return res;
}

int main(int argc, char **argv)
{
    const char *program_name = shift(argv, argc);
    const uint32_t comp = 4;
    bool with_img_colors = false;
    uint32_t color = 0xFFFFFFFF;

    while (argc > 0) {
        const char *flag = argv[0];
        if (strcmp(flag, "--help") == 0) {
            print_usage(program_name);
            return 0;
        } else if (strcmp(flag, "--with-img-colors") == 0) {
            shift(argv, argc); // remove flag from argv
            with_img_colors = true;
        } else if (strcmp(flag, "--with-color") == 0) {
            shift(argv, argc); // remove flag from argv
            if (argc <= 0) {
                fprintf(stderr, "ERROR: No argument provided for '%s'\n", flag);
                return 1;
            }
            color = hextou32(shift(argv, argc));
        } else {
            break;
        }
    }

    if (argc <= 0) {
        fprintf(stderr, "ERROR: No input image provided\n");
        return 1;
    }
    const char *input_path = shift(argv, argc);
    if (argc <= 0) {
        fprintf(stderr, "ERROR: No output image path provided\n");
        return 1;
    }
    const char *output_path = shift(argv, argc);

    size_t width, height;
    uint8_t *pixels = stbi_load(input_path, (int *) &width, (int *) &height, NULL, comp);
    if (!pixels) {
        fprintf(stderr, "ERROR: Could not load input image: %s\n", input_path);
        return 1;
    }
    convert_img_to_ascii(pixels, width, height, comp, color, with_img_colors);

    if (!stbi_write_png(output_path, width, height, comp, pixels, width*comp*sizeof(uint8_t))) {
        fprintf(stderr, "ERROR: Could not save output image: %s\n", output_path);
        return 1;
    }

    free(pixels);
    return 0;
}
