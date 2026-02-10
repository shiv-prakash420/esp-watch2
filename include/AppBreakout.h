#ifndef APP_BREAKOUT_H
#define APP_BREAKOUT_H

#include <lvgl.h>

void AppBreakout_Init();
void AppBreakout_Enter();
void AppBreakout_Start();
void AppBreakout_Stop();
void AppBreakout_Update();
void AppBreakout_MovePaddle(int direction); // -1 = left, 1 = right
lv_obj_t* AppBreakout_GetScreen();
bool AppBreakout_IsActive();
bool AppBreakout_IsInMenu();
bool AppBreakout_IsPlaying();

#endif