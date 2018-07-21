#ifndef COMMON_INCLUDED
#define COMMON_INCLUDED

#include <iostream>

#define LOG(...)  printf("[%s:%d] ", __FILE__, __LINE__); printf(__VA_ARGS__)

#define LOG_STREAM() std::cout << "[" << __FILE__ << ":" << __LINE__ << "] "

//Screen dimension constants  
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;

const float B2_UNITS_TO_PIXELS = 100.0f;
const float PIXELS_TO_B2_UNITS = 1.0f / B2_UNITS_TO_PIXELS;


#endif