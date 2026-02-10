#include <Arduino.h>
#include "AppSnake.h"

// Game constants - OPTIMIZED FOR RECTANGULAR FULL SCREEN
#define GRID_WIDTH 10        // 10 cells wide
#define GRID_HEIGHT 16       // 16 cells tall (rectangular)
#define CELL_SIZE 11         // 11 pixels per cell
#define GRID_OFFSET_X 12     // Center horizontally (135 - 110) / 2
#define GRID_OFFSET_Y 55     // Start below header
#define MAX_SNAKE_LENGTH 160 // 10x16 = 160 max

// Direction constants
#define DIR_UP 0
#define DIR_RIGHT 1
#define DIR_DOWN 2
#define DIR_LEFT 3

// Game state
static lv_obj_t *snake_screen;
static lv_obj_t *score_label;
static lv_obj_t *status_label;
static lv_obj_t *grid_obj;
static lv_obj_t *snake_parts[MAX_SNAKE_LENGTH];
static lv_obj_t *food_obj;

static bool game_active = false;
static bool game_started = false;
static int snake_length = 3;
static int snake_x[MAX_SNAKE_LENGTH];
static int snake_y[MAX_SNAKE_LENGTH];
static int direction = DIR_RIGHT;
static int next_direction = DIR_RIGHT;
static int food_x = 5;
static int food_y = 5;
static int score = 0;
static unsigned long last_move = 0;
static int move_delay = 180;
static unsigned long game_over_time = 0; // ← ADD THIS

// bool AppSnake_IsInMenu()
// {
//     return game_active && !game_started;
// }

// Forward declarations
void AppSnake_GameOver();
void AppSnake_PlaceFood();

void AppSnake_Init()
{
    snake_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(snake_screen, lv_color_hex(0x0a0a0a), 0); // Black background

    // Header container
    lv_obj_t *header = lv_obj_create(snake_screen);
    lv_obj_set_size(header, 135, 50);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x1a1a2e), 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_set_style_radius(header, 0, 0);

    // Title
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "SNAKE");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x00ff41), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 3);

    // Score
    score_label = lv_label_create(header);
    lv_label_set_text(score_label, "Score: 0");
    lv_obj_set_style_text_font(score_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(score_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(score_label, LV_ALIGN_BOTTOM_MID, 0, -3);

    // Grid background - RECTANGULAR FULL SCREEN
    int grid_pixel_width = GRID_WIDTH * CELL_SIZE;
    int grid_pixel_height = GRID_HEIGHT * CELL_SIZE;

    grid_obj = lv_obj_create(snake_screen);
    lv_obj_set_size(grid_obj, grid_pixel_width + 4, grid_pixel_height + 4);
    lv_obj_set_pos(grid_obj, GRID_OFFSET_X - 2, GRID_OFFSET_Y - 2);
    lv_obj_set_style_bg_color(grid_obj, lv_color_hex(0x0f3460), 0);
    lv_obj_set_style_border_color(grid_obj, lv_color_hex(0x00ff41), 0);
    lv_obj_set_style_border_width(grid_obj, 2, 0);
    lv_obj_set_style_pad_all(grid_obj, 0, 0);
    lv_obj_set_style_radius(grid_obj, 0, 0);

    // Status label (centered on grid)
    status_label = lv_label_create(snake_screen);
    lv_label_set_text(status_label, "Press any\narrow to Start\n\n" LV_SYMBOL_OK " Exit");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0x00ff41), 0);
    lv_obj_set_style_text_align(status_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_14, 0);
    lv_obj_align(status_label, LV_ALIGN_CENTER, 0, 30);

    // Create snake parts
    for (int i = 0; i < MAX_SNAKE_LENGTH; i++)
    {
        snake_parts[i] = lv_obj_create(snake_screen);
        lv_obj_set_size(snake_parts[i], CELL_SIZE - 2, CELL_SIZE - 2);
        lv_obj_set_style_bg_color(snake_parts[i], lv_color_hex(0x00ff41), 0);
        lv_obj_set_style_border_width(snake_parts[i], 0, 0);
        lv_obj_set_style_radius(snake_parts[i], 2, 0);
        lv_obj_set_style_pad_all(snake_parts[i], 0, 0);
        lv_obj_add_flag(snake_parts[i], LV_OBJ_FLAG_HIDDEN);
    }

    // Create food
    food_obj = lv_obj_create(snake_screen);
    lv_obj_set_size(food_obj, CELL_SIZE - 2, CELL_SIZE - 2);
    lv_obj_set_style_bg_color(food_obj, lv_color_hex(0xff0000), 0);
    lv_obj_set_style_border_width(food_obj, 0, 0);
    lv_obj_set_style_radius(food_obj, CELL_SIZE / 2, 0);
    lv_obj_set_style_pad_all(food_obj, 0, 0);
    lv_obj_add_flag(food_obj, LV_OBJ_FLAG_HIDDEN);
}

void AppSnake_PlaceFood()
{
    bool valid = false;
    while (!valid)
    {
        food_x = random(0, GRID_WIDTH);
        food_y = random(0, GRID_HEIGHT);

        valid = true;
        for (int i = 0; i < snake_length; i++)
        {
            if (snake_x[i] == food_x && snake_y[i] == food_y)
            {
                valid = false;
                break;
            }
        }
    }

    lv_obj_set_pos(food_obj, GRID_OFFSET_X + food_x * CELL_SIZE, GRID_OFFSET_Y + food_y * CELL_SIZE);
}

void AppSnake_Enter()
{
    game_active = true;
    game_started = false;
    game_over_time = 0;

    // Show start screen
    lv_label_set_text(status_label, "Press center\n to Start\n\n" LV_SYMBOL_OK " Exit");
    lv_obj_clear_flag(status_label, LV_OBJ_FLAG_HIDDEN);

    // Hide game objects
    for (int i = 0; i < MAX_SNAKE_LENGTH; i++)
    {
        lv_obj_add_flag(snake_parts[i], LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_add_flag(food_obj, LV_OBJ_FLAG_HIDDEN);
}

void AppSnake_Start()
{
    game_active = true;
    game_started = true;
    snake_length = 3;
    score = 0;
    direction = DIR_RIGHT;
    next_direction = DIR_RIGHT;
    move_delay = 180;

    // Initialize snake in the middle
    snake_x[0] = GRID_WIDTH / 2;
    snake_y[0] = GRID_HEIGHT / 2;
    snake_x[1] = GRID_WIDTH / 2 - 1;
    snake_y[1] = GRID_HEIGHT / 2;
    snake_x[2] = GRID_WIDTH / 2 - 2;
    snake_y[2] = GRID_HEIGHT / 2;

    // Show snake
    for (int i = 0; i < snake_length; i++)
    {
        lv_obj_clear_flag(snake_parts[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_pos(snake_parts[i],
                       GRID_OFFSET_X + snake_x[i] * CELL_SIZE,
                       GRID_OFFSET_Y + snake_y[i] * CELL_SIZE);
    }

    // Hide remaining parts
    for (int i = snake_length; i < MAX_SNAKE_LENGTH; i++)
    {
        lv_obj_add_flag(snake_parts[i], LV_OBJ_FLAG_HIDDEN);
    }

    // Place food
    AppSnake_PlaceFood();
    lv_obj_clear_flag(food_obj, LV_OBJ_FLAG_HIDDEN);

    // Update UI
    lv_label_set_text(score_label, "Score: 0");
    lv_obj_add_flag(status_label, LV_OBJ_FLAG_HIDDEN);

    last_move = millis();
}

void AppSnake_Stop()
{
    game_active = false;
    game_started = false;
    game_over_time = 0;
    // Hide all game objects
    for (int i = 0; i < MAX_SNAKE_LENGTH; i++)
    {
        lv_obj_add_flag(snake_parts[i], LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_add_flag(food_obj, LV_OBJ_FLAG_HIDDEN);

    // Reset status
    lv_label_set_text(status_label, "Press center\n to Start\n\n" LV_SYMBOL_OK " Exit");
    lv_obj_clear_flag(status_label, LV_OBJ_FLAG_HIDDEN);
}

void AppSnake_SetDirection(int dir)
{
    if (!game_active)
        return;

    // Start game on first direction press
    if (!game_started)
    {
        if (game_over_time > 0 && millis() - game_over_time < 1000)
        {
            return; // Ignore button presses during freeze
        }

        game_over_time = 0;
        AppSnake_Start();
        next_direction = dir;
        direction = dir;
        return;
    }

    // Prevent 180 degree turns
    if (dir == DIR_UP && direction != DIR_DOWN)
        next_direction = dir;
    else if (dir == DIR_DOWN && direction != DIR_UP)
        next_direction = dir;
    else if (dir == DIR_LEFT && direction != DIR_RIGHT)
        next_direction = dir;
    else if (dir == DIR_RIGHT && direction != DIR_LEFT)
        next_direction = dir;
}

void AppSnake_GameOver()
{
    game_started = false;
    game_over_time = millis();
    lv_label_set_text_fmt(status_label, "GAME OVER!\nScore: %d\n\nPress Center\nto Restart\n\n" LV_SYMBOL_OK " Exit", score);
    lv_obj_clear_flag(status_label, LV_OBJ_FLAG_HIDDEN);
}

void AppSnake_Update()
{
    if (!game_active || !game_started)
        return;

    // Check if it's time to move
    if (millis() - last_move < move_delay)
        return;
    last_move = millis();

    // Update direction
    direction = next_direction;

    // Calculate new head position
    int new_x = snake_x[0];
    int new_y = snake_y[0];

    switch (direction)
    {
    case DIR_UP:
        new_y--;
        break;
    case DIR_DOWN:
        new_y++;
        break;
    case DIR_LEFT:
        new_x--;
        break;
    case DIR_RIGHT:
        new_x++;
        break;
    }

    // Check wall collision
    if (new_x < 0 || new_x >= GRID_WIDTH || new_y < 0 || new_y >= GRID_HEIGHT)
    {
        AppSnake_GameOver();
        return;
    }

    // Check self collision
    for (int i = 0; i < snake_length; i++)
    {
        if (snake_x[i] == new_x && snake_y[i] == new_y)
        {
            AppSnake_GameOver();
            return;
        }
    }

    // Check food collision
    bool ate_food = (new_x == food_x && new_y == food_y);

    if (ate_food)
    {
        score++;
        lv_label_set_text_fmt(score_label, "Score: %d", score);

        // Increase speed slightly
        if (move_delay > 60)
            move_delay -= 4;

        // Grow snake
        snake_length++;
        if (snake_length >= MAX_SNAKE_LENGTH)
        {
            // Win!
            lv_label_set_text_fmt(status_label, "YOU WIN!\nPerfect Score!\n\n" LV_SYMBOL_OK " Exit");
            lv_obj_clear_flag(status_label, LV_OBJ_FLAG_HIDDEN);
            game_started = false;
            return;
        }

        // Place new food
        AppSnake_PlaceFood();
    }

    // Move snake body
    for (int i = snake_length - 1; i > 0; i--)
    {
        snake_x[i] = snake_x[i - 1];
        snake_y[i] = snake_y[i - 1];
    }

    // Move head
    snake_x[0] = new_x;
    snake_y[0] = new_y;

    // Update visual positions
    for (int i = 0; i < snake_length; i++)
    {
        lv_obj_clear_flag(snake_parts[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_set_pos(snake_parts[i],
                       GRID_OFFSET_X + snake_x[i] * CELL_SIZE,
                       GRID_OFFSET_Y + snake_y[i] * CELL_SIZE);
    }
}

lv_obj_t *AppSnake_GetScreen()
{
    return snake_screen;
}

bool AppSnake_IsActive()
{
    return game_active;
}
bool AppSnake_IsInMenu()
{
    return game_active && !game_started;
}

bool AppSnake_IsPlaying()
{
    return game_active && game_started;
}