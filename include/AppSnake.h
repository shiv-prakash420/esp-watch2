#ifndef APP_SNAKE_H
#define APP_SNAKE_H

#include <lvgl.h>

// Direction constants
#define DIR_UP 0
#define DIR_RIGHT 1
#define DIR_DOWN 2
#define DIR_LEFT 3

void AppSnake_Init();
void AppSnake_Enter();
void AppSnake_Start();
void AppSnake_Stop();
void AppSnake_Update();
void AppSnake_SetDirection(int dir); // 0=UP, 1=RIGHT, 2=DOWN, 3=LEFT
lv_obj_t *AppSnake_GetScreen();
bool AppSnake_IsActive();
bool AppSnake_IsInMenu();
bool AppSnake_IsPlaying();

#endif