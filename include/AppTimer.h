#ifndef APP_TIMER_H
#define APP_TIMER_H

#include <lvgl.h>

void AppTimer_Init();
void AppTimer_Adjust(int minutes);
void AppTimer_Toggle();
void AppTimer_Reset(); // ← Add this
void AppTimer_Update();
void AppTimer_SetAlarmCallback(void (*callback)());
lv_obj_t *AppTimer_GetScreen();

#endif