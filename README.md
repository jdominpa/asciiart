# ASCII art image converter

Convert any image to an ASCII version of itself. Inspired by [Acerola's
video](https://youtu.be/gg40RWiaHRY?si=BXRhRr5xB4rvH4QU) about ASCII art
shaders.

![cat_output](examples/cat_output.png)

The image above is the result of the input image

![cat](examples/cat.png)

## Quick Start

```console
$ make
$ ./asciiart [options] <input_img> <output.png>
```

Currently the image is always saved in `png` format. Other formats may be added
in the future. The available options are:

| Option            | Description                                                           |
|-------------------|-----------------------------------------------------------------------|
| `-edge_threshold` | Threshold value (0-255) used for edge detection. Default value is 64. |
