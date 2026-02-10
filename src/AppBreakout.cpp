#include <Arduino.h>
#include "AppBreakout.h"

// Game constants
#define GAME_WIDTH 115
#define GAME_HEIGHT 176
#define GAME_OFFSET_X 10
#define GAME_OFFSET_Y 55

#define PADDLE_WIDTH 30
#define PADDLE_HEIGHT 5
#define PADDLE_Y 165
#define PADDLE_SPEED 15

#define BALL_SIZE 5
#define BALL_SPEED 1.3

#define BRICK_ROWS 6
#define BRICK_COLS 8
#define BRICK_WIDTH 13
#define BRICK_HEIGHT 8
#define BRICK_SPACING 1
#define BRICK_OFFSET_X 11
#define BRICK_OFFSET_Y 60

// Game state
static lv_obj_t *breakout_screen;
static lv_obj_t *score_label;
static lv_obj_t *lives_label;
static lv_obj_t *status_label;
static lv_obj_t *game_area;
static lv_obj_t *paddle;
static lv_obj_t *ball;
static lv_obj_t *bricks[BRICK_ROWS][BRICK_COLS];

static bool game_active = false;
static bool game_started = false;
static float paddle_x = (GAME_WIDTH - PADDLE_WIDTH) / 2;
static float ball_x = GAME_WIDTH / 2;
static float ball_y = PADDLE_Y - 20;
static float ball_vel_x = BALL_SPEED;
static float ball_vel_y = -BALL_SPEED;
static int score = 0;
static int lives = 3;
static int bricks_remaining = 0;
static bool brick_active[BRICK_ROWS][BRICK_COLS];
static unsigned long game_over_time = 0;

// Colors for brick rows (rainbow pattern)
static const uint32_t brick_colors[BRICK_ROWS] = {
    0xFF0000,  // Red
    0xFF7F00,  // Orange
    0xFFFF00,  // Yellow
    0x00FF00,  // Green
    0x0000FF,  // Blue
    0x8B00FF   // Purple
};

// Forward declarations
void AppBreakout_GameOver();
void AppBreakout_ResetBall();
void AppBreakout_Win();

void AppBreakout_Init() {
    breakout_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(breakout_screen, lv_color_hex(0x0a0a0a), 0);

    // Header
    lv_obj_t *header = lv_obj_create(breakout_screen);
    lv_obj_set_size(header, 135, 50);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x1a1a2e), 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_set_style_radius(header, 0, 0);

    // Title
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "BREAKOUT");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFF00FF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 3);

    // Score
    score_label = lv_label_create(header);
    lv_label_set_text(score_label, "Score: 0");
    lv_obj_set_style_text_font(score_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(score_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(score_label, LV_ALIGN_BOTTOM_LEFT, 5, -3);

    // Lives
    lives_label = lv_label_create(header);
    lv_label_set_text(lives_label, "Lives: 3");
    lv_obj_set_style_text_font(lives_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(lives_label, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(lives_label, LV_ALIGN_BOTTOM_RIGHT, -5, -3);

    // Game area background
    game_area = lv_obj_create(breakout_screen);
    lv_obj_set_size(game_area, GAME_WIDTH + 4, GAME_HEIGHT + 4);
    lv_obj_set_pos(game_area, GAME_OFFSET_X - 2, GAME_OFFSET_Y - 2);
    lv_obj_set_style_bg_color(game_area, lv_color_hex(0x000000), 0);
    lv_obj_set_style_border_color(game_area, lv_color_hex(0xFF00FF), 0);
    lv_obj_set_style_border_width(game_area, 2, 0);
    lv_obj_set_style_pad_all(game_area, 0, 0);
    lv_obj_set_style_radius(game_area, 0, 0);

    // Status label
    status_label = lv_label_create(breakout_screen);
    lv_label_set_text(status_label, "Press " LV_SYMBOL_PLAY "\nto Start\n\n" LV_SYMBOL_LEFT LV_SYMBOL_RIGHT " Navigate");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xFF00FF), 0);
    lv_obj_set_style_text_align(status_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_14, 0);
    lv_obj_align(status_label, LV_ALIGN_CENTER, 0, 30);

    // Paddle
    paddle = lv_obj_create(breakout_screen);
    lv_obj_set_size(paddle, PADDLE_WIDTH, PADDLE_HEIGHT);
    lv_obj_set_style_bg_color(paddle, lv_color_hex(0x00FFFF), 0);
    lv_obj_set_style_border_width(paddle, 0, 0);
    lv_obj_set_style_radius(paddle, 2, 0);
    lv_obj_add_flag(paddle, LV_OBJ_FLAG_HIDDEN);

    // Ball
    ball = lv_obj_create(breakout_screen);
    lv_obj_set_size(ball, BALL_SIZE, BALL_SIZE);
    lv_obj_set_style_bg_color(ball, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_width(ball, 0, 0);
    lv_obj_set_style_radius(ball, BALL_SIZE/2, 0);
    lv_obj_add_flag(ball, LV_OBJ_FLAG_HIDDEN);

    // Create bricks
    for(int row = 0; row < BRICK_ROWS; row++) {
        for(int col = 0; col < BRICK_COLS; col++) {
            bricks[row][col] = lv_obj_create(breakout_screen);
            lv_obj_set_size(bricks[row][col], BRICK_WIDTH, BRICK_HEIGHT);
            lv_obj_set_style_bg_color(bricks[row][col], lv_color_hex(brick_colors[row]), 0);
            lv_obj_set_style_border_width(bricks[row][col], 0, 0);
            lv_obj_set_style_radius(bricks[row][col], 1, 0);
            lv_obj_add_flag(bricks[row][col], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void AppBreakout_Enter() {
    game_active = true;
    game_started = false;
    game_over_time = 0;
    
    lv_label_set_text(status_label, "Press " LV_SYMBOL_PLAY "\nto Start\n\n" LV_SYMBOL_LEFT LV_SYMBOL_RIGHT " Navigate");
    lv_obj_clear_flag(status_label, LV_OBJ_FLAG_HIDDEN);
    
    lv_obj_add_flag(paddle, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ball, LV_OBJ_FLAG_HIDDEN);
    
    for(int row = 0; row < BRICK_ROWS; row++) {
        for(int col = 0; col < BRICK_COLS; col++) {
            lv_obj_add_flag(bricks[row][col], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

void AppBreakout_ResetBall() {
    ball_x = GAME_WIDTH / 2;
    ball_y = PADDLE_Y - 20;
    ball_vel_x = (random(0, 2) ? BALL_SPEED : -BALL_SPEED);
    ball_vel_y = -BALL_SPEED;
}

void AppBreakout_Start() {
    game_active = true;
    game_started = true;
    game_over_time = 0;
    score = 0;
    lives = 3;
    bricks_remaining = BRICK_ROWS * BRICK_COLS;
    
    // Reset paddle
    paddle_x = (GAME_WIDTH - PADDLE_WIDTH) / 2;
    
    // Reset ball
    AppBreakout_ResetBall();
    
    // Show all bricks
    for(int row = 0; row < BRICK_ROWS; row++) {
        for(int col = 0; col < BRICK_COLS; col++) {
            brick_active[row][col] = true;
            lv_obj_clear_flag(bricks[row][col], LV_OBJ_FLAG_HIDDEN);
            int x = BRICK_OFFSET_X + col * (BRICK_WIDTH + BRICK_SPACING);
            int y = BRICK_OFFSET_Y + row * (BRICK_HEIGHT + BRICK_SPACING);
            lv_obj_set_pos(bricks[row][col], x, y);
        }
    }
    
    // Show game objects
    lv_obj_clear_flag(paddle, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(ball, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(status_label, LV_OBJ_FLAG_HIDDEN);
    
    // Update UI
    lv_label_set_text(score_label, "Score: 0");
    lv_label_set_text(lives_label, "Lives: 3");
}

void AppBreakout_Stop() {
    game_active = false;
    game_started = false;
    game_over_time = 0;
    
    lv_obj_add_flag(paddle, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(ball, LV_OBJ_FLAG_HIDDEN);
    
    for(int row = 0; row < BRICK_ROWS; row++) {
        for(int col = 0; col < BRICK_COLS; col++) {
            lv_obj_add_flag(bricks[row][col], LV_OBJ_FLAG_HIDDEN);
        }
    }
    
    lv_label_set_text(status_label, "Press " LV_SYMBOL_PLAY "\nto Start\n\n" LV_SYMBOL_LEFT LV_SYMBOL_RIGHT " Navigate");
    lv_obj_clear_flag(status_label, LV_OBJ_FLAG_HIDDEN);
}

void AppBreakout_MovePaddle(int direction) {
    if(!game_started) {
        // Check freeze period
        if(game_over_time > 0 && millis() - game_over_time < 1000) {
            return;
        }
        game_over_time = 0;
        AppBreakout_Start();
        return;
    }
    
    paddle_x += direction * PADDLE_SPEED;
    
    // Keep paddle in bounds
    if(paddle_x < 0) paddle_x = 0;
    if(paddle_x > GAME_WIDTH - PADDLE_WIDTH) paddle_x = GAME_WIDTH - PADDLE_WIDTH;
}

void AppBreakout_GameOver() {
    game_started = false;
    game_over_time = millis();
    
    lv_label_set_text_fmt(status_label, "GAME OVER!\nScore: %d\n\n" LV_SYMBOL_PLAY " Restart\n" LV_SYMBOL_LEFT LV_SYMBOL_RIGHT " Navigate", score);
    lv_obj_clear_flag(status_label, LV_OBJ_FLAG_HIDDEN);
}

void AppBreakout_Win() {
    game_started = false;
    game_over_time = millis();
    
    lv_label_set_text_fmt(status_label, "YOU WIN!\nScore: %d\n\n" LV_SYMBOL_PLAY " Restart\n" LV_SYMBOL_LEFT LV_SYMBOL_RIGHT " Navigate", score);
    lv_obj_clear_flag(status_label, LV_OBJ_FLAG_HIDDEN);
}

void AppBreakout_Update() {
    if(!game_active || !game_started) return;
    
    // Update ball position
    ball_x += ball_vel_x;
    ball_y += ball_vel_y;
    
    // Wall collision (left/right)
    if(ball_x <= 0 || ball_x >= GAME_WIDTH - BALL_SIZE) {
        ball_vel_x = -ball_vel_x;
        ball_x = constrain(ball_x, 0, GAME_WIDTH - BALL_SIZE);
    }
    
    // Ceiling collision
    if(ball_y <= 0) {
        ball_vel_y = -ball_vel_y;
        ball_y = 0;
    }
    
    // Paddle collision
    if(ball_y + BALL_SIZE >= PADDLE_Y && 
       ball_y + BALL_SIZE <= PADDLE_Y + PADDLE_HEIGHT &&
       ball_x + BALL_SIZE >= paddle_x && 
       ball_x <= paddle_x + PADDLE_WIDTH) {
        
        ball_vel_y = -abs(ball_vel_y);
        
        // Add spin based on where ball hits paddle
        float hit_pos = (ball_x + BALL_SIZE/2 - paddle_x) / PADDLE_WIDTH;
        ball_vel_x = (hit_pos - 0.5) * BALL_SPEED * 2;
    }
    
    // Ball falls below paddle - lose life
    if(ball_y > GAME_HEIGHT) {
        lives--;
        lv_label_set_text_fmt(lives_label, "Lives: %d", lives);
        
        if(lives <= 0) {
            AppBreakout_GameOver();
            return;
        }
        
        AppBreakout_ResetBall();
    }
    
    // Brick collision
    for(int row = 0; row < BRICK_ROWS; row++) {
        for(int col = 0; col < BRICK_COLS; col++) {
            if(!brick_active[row][col]) continue;
            
            int brick_x = BRICK_OFFSET_X + col * (BRICK_WIDTH + BRICK_SPACING) - GAME_OFFSET_X;
            int brick_y = BRICK_OFFSET_Y + row * (BRICK_HEIGHT + BRICK_SPACING) - GAME_OFFSET_Y;
            
            // Check collision
            if(ball_x + BALL_SIZE >= brick_x && 
               ball_x <= brick_x + BRICK_WIDTH &&
               ball_y + BALL_SIZE >= brick_y && 
               ball_y <= brick_y + BRICK_HEIGHT) {
                
                // Destroy brick
                brick_active[row][col] = false;
                lv_obj_add_flag(bricks[row][col], LV_OBJ_FLAG_HIDDEN);
                
                // Update score (higher rows = more points)
                score += (BRICK_ROWS - row) * 10;
                lv_label_set_text_fmt(score_label, "Score: %d", score);
                
                // Bounce ball
                ball_vel_y = -ball_vel_y;
                
                bricks_remaining--;
                
                // Check win
                if(bricks_remaining <= 0) {
                    AppBreakout_Win();
                    return;
                }
                
                break;
            }
        }
    }
    
    // Update visual positions
    lv_obj_set_pos(paddle, GAME_OFFSET_X + (int)paddle_x, GAME_OFFSET_Y + PADDLE_Y);
    lv_obj_set_pos(ball, GAME_OFFSET_X + (int)ball_x, GAME_OFFSET_Y + (int)ball_y);
}

lv_obj_t* AppBreakout_GetScreen() {
    return breakout_screen;
}

bool AppBreakout_IsActive() {
    return game_active;
}

bool AppBreakout_IsInMenu() {
    return game_active && !game_started;
}

bool AppBreakout_IsPlaying() {
    return game_active && game_started;
}