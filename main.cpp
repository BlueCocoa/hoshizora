//
//  main.cpp
//  hoshizora
//
//  Created by BlueCocoa on 2016/9/5.
//  Copyright © 2016年 BlueCocoa. All rights reserved.
//

#include <getopt.h>
#include <stdlib.h>
#include <cmath>
#include <opencv2/opencv.hpp>
#include <opencv2/core/version.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <libpng16/png.h>

using namespace cv;

#if (CV_VERSION_MAJOR < 3)
#define CV_CVTCOLOR_BGR2GREY  CV_BGR2GRAY
#define CV_CVTCOLOR_GREY2BGRA CV_GRAY2BGRA
#else
#define CV_CVTCOLOR_BGR2GREY  COLOR_BGR2GRAY
#define CV_CVTCOLOR_GREY2BGRA COLOR_GRAY2BGRA
#endif

#define K_TRANSPARENT_ALPHA 0

#define ENSURE_NOT_NULL(x) (x != nullptr)

char * front_file = NULL;
char * back_file = NULL;
char * output = NULL;

static struct option long_options[] = {
    {"help",            no_argument,       0, 'h'},
    {"front",           required_argument, 0, 'f'},
    {"back",            required_argument, 0, 'b'},
    {"ouput",           required_argument, 0, 'o'},
    {0, 0, 0, 0}
};

/**
 *  @brief Prints the usage message of this program.
 */
void print_usage() {
    fprintf(stderr, "Usage: hoshizora [-f front layer]\n"
                    "                 [-b back layer]\n"
                    "                 [-o ouput]\n"
                    "\n"
                    "                 -h To print this help\n");
}

int parse_command_line(int argc, char * const * argv) {
    int c;
    int option_index = 0;
    
    while (1) {
        option_index = 0;
        c = getopt_long (argc, argv, "hf:b:o:", long_options, &option_index);
        if (c == -1)
            break;
        switch (c) {
            case 0:
                if (long_options[option_index].flag != 0)
                    break;
                break;
            case 'h':
                return 0;
            case 'o': {
                asprintf(&output, "%s", optarg);
                break;
            }
            case 'f': {
                asprintf(&front_file, "%s", optarg);
                break;
            }
            case 'b': {
                asprintf(&back_file, "%s", optarg);
                break;
            }
            case '?':
                return 0;
            default:
                abort();
        }
    }
    
    return ENSURE_NOT_NULL(front_file)    &&
           ENSURE_NOT_NULL(back_file)     &&
           ENSURE_NOT_NULL(output);
}

/**
 *  @brief Resize image to fit
 */
void resize(Mat &src, Mat &dest, int width, int height, int interpolation = INTER_LINEAR) {
    if (width <= 0 && height <= 0) {
        dest = src.clone();
        return;
    }
    int w,h;
    w = src.cols;
    h = src.rows;
    
    if (width == 0) {
        double ratio = height / double(h);
        width = w * ratio;
    } else {
        double ratio = width / double(w);
        height = h * ratio;
    }
    cv::resize(src, dest, Size(width, height), width/w, height/h, interpolation);
}

int magic(char* filename, Size size, Mat& frontlayer, Mat& backlayer) {
    int code = 0;
    FILE *fp = NULL;
    png_structp png_ptr = NULL;
    png_infop info_ptr = NULL;
    png_bytep row = NULL;
    auto front_pixel = frontlayer.begin<Vec<uchar, 1>>();
    auto back_pixel = backlayer.begin<Vec<uchar, 1>>();
    
    // Open file for writing (binary mode)
    fp = fopen(filename, "wb");
    if (fp == NULL) {
        fprintf(stderr, "Could not open file %s for writing\n", filename);
        code = 1;
        goto finalise;
    }
    
    // Initialize write structure
    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        fprintf(stderr, "Could not allocate write struct\n");
        code = 1;
        goto finalise;
    }
    
    // Initialize info structure
    info_ptr = png_create_info_struct(png_ptr);
    if (info_ptr == NULL) {
        fprintf(stderr, "Could not allocate info struct\n");
        code = 1;
        goto finalise;
    }
    
    // Setup Exception handling
    if (setjmp(png_jmpbuf(png_ptr))) {
        fprintf(stderr, "Error during png creation\n");
        code = 1;
        goto finalise;
    }
    png_init_io(png_ptr, fp);
    
    // Write header (8 bit colour depth)
    png_set_IHDR(png_ptr, info_ptr, size.width, size.height,
                 8, PNG_COLOR_TYPE_GRAY_ALPHA, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    
    png_write_info(png_ptr, info_ptr);
    
    // Allocate memory for one row (2 bytes per pixel - Gray+Alpha)
    row = (png_bytep)malloc(2 * size.width * sizeof(png_byte));
    
    // Write image data
    
    for (int y = 0; y < size.height; y++) {
        for (int x = 0; x < size.width; x++) {
            uchar _y = (*back_pixel)[0];
            uchar _x = (*front_pixel)[0];
            uchar A = min(_y + 255 - _x, 255);
            uchar G = (_y * 255.0f) / A;
            row[x * 2 + 1] = A;
            row[x * 2]     = G;
            front_pixel++;
            back_pixel++;
        }
        png_write_row(png_ptr, row);
    }
    
    // End write
    png_write_end(png_ptr, NULL);
finalise:
    if (fp != NULL) fclose(fp);
    if (info_ptr != NULL) png_free_data(png_ptr, info_ptr, PNG_FREE_ALL, -1);
    if (png_ptr != NULL) png_destroy_write_struct(&png_ptr, (png_infopp)NULL);
    if (row != NULL) free(row);
    
    return code;
}

void overlay_center(Mat& bottom, Mat& layer, double alpha = 0) {
    Rect roi = Rect((bottom.cols - layer.cols) / 2, (bottom.rows - layer.rows) / 2, layer.cols, layer.rows);
    layer.copyTo(bottom(roi));
}

int main(int argc, const char * argv[]) {
    if (!parse_command_line(argc, (char * const *)argv)) {
        print_usage();
        exit(EXIT_FAILURE);
    }
    
    // Load and transform to gray scale
    Mat front = imread(front_file, CV_LOAD_IMAGE_GRAYSCALE);
    Mat back  = imread(back_file,  CV_LOAD_IMAGE_GRAYSCALE);

    // resize image to fit
    if (front.cols > back.cols) {
        if (front.rows > back.rows) {
            if (((double)front.cols / (double)front.rows) > ((double)back.cols / (double)back.rows)) {
                resize(back, back, 0, front.rows);
            } else {
                resize(back, back, front.cols, 0);
            }
        } else {
            resize(back, back, 0, front.rows);
        }
    } else {
        if (front.rows < back.rows) {
            if (((double)front.cols / (double)front.rows) > ((double)back.cols / (double)back.rows)) {
                resize(front, front, 0, back.rows);
            } else {
                resize(front, front, back.cols, 0);
            }
        } else {
            resize(front, front, 0, back.rows);
        }
    }

    // new layers, same size
    auto size = Size(max(front.cols, back.cols), max(front.rows, back.rows));
    
    Mat frontlayer(size, CV_8U, Scalar(255));
    Mat backlayer(size, CV_8U, Scalar(0));
    
    overlay_center(frontlayer, front);
    overlay_center(backlayer, back);
    
    magic(output, size, frontlayer, backlayer);

    free((void *)front_file);
    free((void *)back_file);
    free((void *)output);
    return 0;
}
