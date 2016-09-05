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

using namespace cv;

#if (CV_VERSION_MAJOR < 3)
#define CV_CVTCOLOR_BGR2GREY  CV_BGR2GRAY
#define CV_CVTCOLOR_GREY2BGRA CV_GRAY2BGRA
#else
#define CV_CVTCOLOR_BGR2GREY  COLOR_BGR2GRAY
#define CV_CVTCOLOR_GREY2BGRA COLOR_GRAY2BGRA
#endif

#define K_DEFAULT_THRESHOLD 192
#define K_TRANSPARENT_ALPHA 0
#define deviant 255

#define ENSURE_NOT_NULL(x) (x != nullptr)

char * frontlayer = NULL;
char * backlayer = NULL;
char * output = NULL;
int threshold_value = K_DEFAULT_THRESHOLD;

static struct option long_options[] = {
    {"help",      no_argument,       0, 'h'},
    {"front",     required_argument, 0, 'f'},
    {"back",      required_argument, 0, 'b'},
    {"ouput",     required_argument, 0, 'o'},
    {"threshold", optional_argument, 0, 't'},
    {0, 0, 0, 0}
};

/**
 *  @brief Prints the usage message of this program.
 */
void print_usage() {
    fprintf(stderr, "Usage: hoshizora [-f front layer] [-b back layer] [-t threshold] [-o ouput]\n"
                    "                 -h To print this help\n");
}

int parse_command_line(int argc, char * const * argv) {
    int c;
    int option_index = 0;
    
    while (1) {
        option_index = 0;
        c = getopt_long (argc, argv, "hf:b:t:o:", long_options, &option_index);
        if (c == -1)
            break;
        switch (c) {
            case 0:
                if (long_options[option_index].flag != 0)
                    break;
                break;
            case 'h':
                print_usage();
                return 0;
            case 'o': {
                asprintf(&output, "%s", optarg);
                break;
            }
            case 'f': {
                asprintf(&frontlayer, "%s", optarg);
                break;
            }
            case 'b': {
                asprintf(&backlayer, "%s", optarg);
                break;
            }
            case 't': {
                threshold_value = atoi(optarg);
                break;
            }
            case '?':
                print_usage();
                return 0;
            default:
                abort();
        }
    }
    
    return ENSURE_NOT_NULL(frontlayer) &&
           ENSURE_NOT_NULL(backlayer)  &&
           ENSURE_NOT_NULL(output) &&
           threshold_value >= 0;
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

int main(int argc, const char * argv[]) {
    if (!parse_command_line(argc, (char * const *)argv)) {
        print_usage();
        exit(EXIT_FAILURE);
    }
    
    // Load and transform to gray scale
    Mat front = imread(frontlayer, CV_LOAD_IMAGE_GRAYSCALE);
    Mat back  = imread(backlayer,  CV_LOAD_IMAGE_GRAYSCALE);
    
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

    // convert colorspace into BGRA
    cvtColor(front, front, CV_CVTCOLOR_GREY2BGRA);
    cvtColor(back,  back,  CV_CVTCOLOR_GREY2BGRA);
    
    // for front layer
    // we remove 'white color' (above threshold)
    for (auto pixel = front.begin<Vec4b>(); pixel != front.end<Vec4b>(); pixel++) {
        if ((*pixel)[0] < threshold_value) {
            (*pixel)[2] = (*pixel)[1] = (*pixel)[0] = 0;
            (*pixel)[3] = 255;
        } else {
            (*pixel)[2] = (*pixel)[1] = (*pixel)[0] = 255;
            (*pixel)[3] = K_TRANSPARENT_ALPHA;
        }
    }
    
    // for back layer
    // we remove 'black color' (below threshold)
    for (auto pixel = back.begin<Vec4b>(); pixel != back.end<Vec4b>(); pixel++) {
        if ((*pixel)[0] > threshold_value) {
            (*pixel)[2] = (*pixel)[1] = (*pixel)[0] = 255;
            (*pixel)[3] = 255;
        } else {
            (*pixel)[2] = (*pixel)[1] = (*pixel)[0] = 0;
            (*pixel)[3] = K_TRANSPARENT_ALPHA;
        }
    }
    
    Mat result = front.clone();
    Rect roi = Rect((front.cols - back.cols) / 2, (front.rows - back.rows) / 2, back.cols, back.rows);
    Mat fusion = result(roi);
    cv::addWeighted(fusion, 0.5, back, 0.5, 0.0, fusion);
    
    imwrite(output, result);

    free((void *)frontlayer);
    free((void *)backlayer);
    free((void *)output);
    return 0;
}
