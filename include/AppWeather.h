#ifndef APP_WEATHER_H
#define APP_WEATHER_H

#include <lvgl.h>

void AppWeather_Init();        // Create the screen and UI
void AppWeather_Update();      // Fetch new data from internet
lv_obj_t* AppWeather_GetScreen(); // Get the screen pointer for navigation

#endif