
#include <Arduino.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <time.h>

#include "bg_image.h"
#include "AppWeather.h"
#include "AppTimer.h"
#include "AppSnake.h"
#include "AppBreakout.h"

LV_FONT_DECLARE(lv_font_montserrat_14);
LV_FONT_DECLARE(lv_font_montserrat_24);
LV_FONT_DECLARE(lv_font_montserrat_48);

lv_obj_t *home_screen; // Variable to store your Clock screen
bool onWeatherPage = false;
bool onTimerPage = false;
bool onSnakePage = false;
bool onBreakoutPage = false;

// --- Hardware Pins (Gamepad Setup) ---
#define BUTTON_PIN 34 // Analog pin for all buttons
#define BUZZER_PIN 25 // Buzzer pin

// Button ADC thresholds based on your measurements
//  center 289, up 564, down 920, right 1620, left 4095
#define BTN_NONE_MIN 0
#define BTN_NONE_MAX 200

#define BTN_CENTER_MIN 220
#define BTN_CENTER_MAX 320

#define BTN_UP_MIN 500
#define BTN_UP_MAX 620

#define BTN_DOWN_MIN 850
#define BTN_DOWN_MAX 1000

#define BTN_RIGHT_MIN 1550
#define BTN_RIGHT_MAX 1650

#define BTN_LEFT_MIN 3995
#define BTN_LEFT_MAX 4095

enum Button
{
  BTN_NONE,
  BTN_UP,
  BTN_DOWN,
  BTN_LEFT,
  BTN_RIGHT,
  BTN_CENTER
};

// ========== BUZZER FUNCTIONS ==========
void beep(int duration_ms = 100)
{
  digitalWrite(BUZZER_PIN, HIGH);
  delay(duration_ms);
  digitalWrite(BUZZER_PIN, LOW);
}

void beepPattern(int count, int duration = 50, int pause = 50)
{
  for (int i = 0; i < count; i++)
  {
    digitalWrite(BUZZER_PIN, HIGH);
    delay(duration);
    digitalWrite(BUZZER_PIN, LOW);
    if (i < count - 1)
      delay(pause);
  }
}

void timerAlarmSound()
{
  // Loud repeating alarm pattern
  for (int i = 0; i < 10; i++)
  {
    beepPattern(3, 100, 50); // 3 quick beeps
    delay(300);
    lv_timer_handler(); // Keep display responsive
  }
}

// ========== BUTTON READING ==========
Button readButton()
{
  int value = analogRead(BUTTON_PIN);

  // Uncomment for debugging
  // Serial.printf("ADC: %d\n", value);

  if (value >= BTN_LEFT_MIN && value <= BTN_LEFT_MAX)
    return BTN_LEFT;
  if (value >= BTN_RIGHT_MIN && value <= BTN_RIGHT_MAX)
    return BTN_RIGHT;
  if (value >= BTN_DOWN_MIN && value <= BTN_DOWN_MAX)
    return BTN_DOWN;
  if (value >= BTN_UP_MIN && value <= BTN_UP_MAX)
    return BTN_UP;
  if (value >= BTN_CENTER_MIN && value <= BTN_CENTER_MAX)
    return BTN_CENTER;
  if (value >= BTN_NONE_MIN && value <= BTN_NONE_MAX)
    return BTN_NONE;

  return BTN_NONE;
}
// --- Global Objects ---
TFT_eSPI tft = TFT_eSPI();
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[135 * 40]; // Larger buffer for smoother image loading
static lv_obj_t *time_label;
static lv_obj_t *date_label;

// --- WiFi Credentials ---
const char *ssid = "Redmi 9 Power";
const char *pass = "890890890";

// --- LVGL Display Flush Function ---
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)&color_p->full, w * h, true);
  tft.endWrite();
  lv_disp_flush_ready(disp);
}

// --- UI Creation ---
void create_watch_face()
{
  lv_obj_t *scr = lv_scr_act();

  // 1. Draw Background Image (adam.jpg)
  static lv_img_dsc_t img_dsc;
  img_dsc.header.always_zero = 0;
  img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR; // RGB565
  img_dsc.header.w = 135;
  img_dsc.header.h = 240;
  img_dsc.data = (const uint8_t *)my_image_map;
  img_dsc.data_size = 64800;

  lv_obj_t *bg = lv_img_create(scr);
  lv_img_set_src(bg, &img_dsc);
  lv_obj_align(bg, LV_ALIGN_CENTER, 0, 0);

  // 2. Create Glass-Morphism Overlay (Makes text readable)
  lv_obj_t *glass = lv_obj_create(scr);
  lv_obj_set_size(glass, 120, 80);
  lv_obj_align(glass, LV_ALIGN_TOP_MID, 0, 20);
  lv_obj_set_style_bg_opa(glass, LV_OPA_40, 0); // Transparent
  lv_obj_set_style_bg_color(glass, lv_color_hex(0x000000), 0);
  lv_obj_set_style_border_width(glass, 0, 0);
  lv_obj_set_style_radius(glass, 10, 0);

  // 3. Time Label
  time_label = lv_label_create(glass);
  lv_obj_set_style_text_font(time_label, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(time_label, lv_color_hex(0xFFFFFF), 0);
  lv_obj_align(time_label, LV_ALIGN_TOP_MID, 0, 5);
  lv_label_set_text(time_label, "00:00");

  // 4. Date Label
  date_label = lv_label_create(glass);
  lv_obj_set_style_text_font(date_label, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(date_label, lv_color_hex(0xCCCCCC), 0);
  lv_obj_align(date_label, LV_ALIGN_BOTTOM_MID, 0, -10);
  lv_label_set_text(date_label, "Loading...");
}

void update_time()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
    return;

  char buf_time[10];
  strftime(buf_time, sizeof(buf_time), "%H:%M", &timeinfo);
  lv_label_set_text(time_label, buf_time);

  char buf_date[20];
  strftime(buf_date, sizeof(buf_date), "%a, %d %b", &timeinfo);
  lv_label_set_text(date_label, buf_date);
}

void setup()
{
  Serial.begin(115200);

  // Hardware Init
  tft.begin();
  tft.setRotation(2);

  // LVGL Init
  lv_init();
  lv_disp_draw_buf_init(&draw_buf, buf, NULL, 135 * 40);
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = 135;
  disp_drv.ver_res = 240;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  // Save the current screen as "Home"
  home_screen = lv_scr_act();

  // UI
  create_watch_face();

  // Buzzer Setup - BEFORE WiFi for startup sound
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  beepPattern(2, 80, 100); // Startup sound: beep-beep

  // WiFi - WAIT FOR CONNECTION
  Serial.println("Connecting to WiFi...");
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    lv_timer_handler(); // Keep display responsive
  }
  Serial.println("\nWiFi Connected!");
  beep(250); // WiFi connected confirmation beep

  // Time sync
  configTime(19800, 0, "pool.ntp.org");
  delay(2000);

  // Initialize Apps
  AppWeather_Init();
  AppTimer_Init();
  AppSnake_Init();
  AppBreakout_Init();

  AppTimer_SetAlarmCallback(timerAlarmSound); // Set timer alarm sound

  // Update weather
  AppWeather_Update();

  // Button Pin (Analog)
  pinMode(BUTTON_PIN, INPUT);

  Serial.println("Setup complete!");
  beep(50); // Final ready beep
}

void loop()
{
  lv_timer_handler();

  // Update time every second (not in game)
  static unsigned long last_tick = 0;
  if (millis() - last_tick > 1000 && !onSnakePage && !onBreakoutPage)
  {
    update_time();
    last_tick = millis();
  }

  // ========== BUTTON READING ==========
  static Button last_button = BTN_NONE;
  static unsigned long last_press = 0;

  Button current_button = readButton();

  if (current_button != last_button && millis() - last_press > 100)
  {
    last_press = millis();

    if (current_button != BTN_NONE)
    {
      beep(20);

      // ========== SNAKE PAGE HANDLING ==========
      if (onSnakePage)
      {
        bool playing = AppSnake_IsPlaying();
        bool in_menu = AppSnake_IsInMenu();

        if (playing)
        {
          // ===== PLAYING: Arrow keys control snake =====
          if (current_button == BTN_UP)
          {
            AppSnake_SetDirection(0);
          }
          else if (current_button == BTN_DOWN)
          {
            AppSnake_SetDirection(2);
          }
          else if (current_button == BTN_LEFT)
          {
            AppSnake_SetDirection(3);
          }
          else if (current_button == BTN_RIGHT)
          {
            AppSnake_SetDirection(1);
          }
        }
        else if (in_menu)
        {
          // ===== MENU: CENTER starts, LEFT/RIGHT navigate =====
          if (current_button == BTN_CENTER)
          {
            AppSnake_Start();
            beep(40);
          }
          else if (current_button == BTN_LEFT)  // ← FIXED: Go to Breakout
          {
            AppSnake_Stop();
            lv_scr_load_anim(AppBreakout_GetScreen(), LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
            onSnakePage = false;
            onBreakoutPage = true;
            AppBreakout_Enter();
          }
          else if (current_button == BTN_RIGHT)  // ← FIXED: Go to Home
          {
            AppSnake_Stop();
            lv_scr_load_anim(home_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
            onSnakePage = false;
          }
        }
        else
        {
          // ===== GAME OVER: CENTER restarts, LEFT/RIGHT navigate =====
          if (current_button == BTN_CENTER)
          {
            AppSnake_SetDirection(DIR_RIGHT); // Trigger restart
            beep(40);
          }
          else if (current_button == BTN_LEFT)  // ← FIXED: Go to Breakout
          {
            AppSnake_Stop();
            lv_scr_load_anim(AppBreakout_GetScreen(), LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
            onSnakePage = false;
            onBreakoutPage = true;
            AppBreakout_Enter();
          }
          else if (current_button == BTN_RIGHT)  // ← FIXED: Go to Home
          {
            AppSnake_Stop();
            lv_scr_load_anim(home_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
            onSnakePage = false;
          }
        }
      }
      
      // ========== BREAKOUT PAGE HANDLING ==========
      else if (onBreakoutPage)
      {
        bool playing = AppBreakout_IsPlaying();
        bool in_menu = AppBreakout_IsInMenu();

        if (playing)
        {
          // ===== PLAYING: LEFT/RIGHT control paddle =====
          if (current_button == BTN_LEFT)
          {
            AppBreakout_MovePaddle(-1);
          }
          else if (current_button == BTN_RIGHT)
          {
            AppBreakout_MovePaddle(1);
          }
        }
        else if (in_menu)
        {
          // ===== MENU: CENTER starts, LEFT/RIGHT navigate =====
          if (current_button == BTN_CENTER)
          {
            AppBreakout_Start();
            beep(40);
          }
          else if (current_button == BTN_RIGHT)  // Go to Snake
          {
            AppBreakout_Stop();
            lv_scr_load_anim(AppSnake_GetScreen(), LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
            onBreakoutPage = false;
            onSnakePage = true;
            AppSnake_Enter();
          }
          else if (current_button == BTN_LEFT)  // ← Can't go further left
          {
            beep(10);  // Just beep, at the edge
          }
        }
        else
        {
          // ===== GAME OVER: CENTER restarts, LEFT/RIGHT navigate =====
          if (current_button == BTN_CENTER)
          {
            AppBreakout_MovePaddle(0); // Trigger restart
            beep(40);
          }
          else if (current_button == BTN_RIGHT)  // Go to Snake
          {
            AppBreakout_Stop();
            lv_scr_load_anim(AppSnake_GetScreen(), LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
            onBreakoutPage = false;
            onSnakePage = true;
            AppSnake_Enter();
          }
          else if (current_button == BTN_LEFT)  // ← Can't go further left
          {
            beep(10);  // Just beep, at the edge
          }
        }
      }
      
      // ========== NORMAL PAGE NAVIGATION ==========
      else
      {
        // LEFT from Home -> Snake
        if (current_button == BTN_LEFT && !onWeatherPage && !onTimerPage)
        {
          lv_scr_load_anim(AppSnake_GetScreen(), LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
          onSnakePage = true;
          AppSnake_Enter();
        }
        // RIGHT from Home -> Weather
        else if (current_button == BTN_RIGHT && !onWeatherPage && !onTimerPage)
        {
          lv_scr_load_anim(AppWeather_GetScreen(), LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
          onWeatherPage = true;
        }
        // RIGHT from Weather -> Timer
        else if (current_button == BTN_RIGHT && onWeatherPage && !onTimerPage)
        {
          lv_scr_load_anim(AppTimer_GetScreen(), LV_SCR_LOAD_ANIM_MOVE_LEFT, 300, 0, false);
          onWeatherPage = false;
          onTimerPage = true;
        }
        // LEFT from Timer -> Weather
        else if (current_button == BTN_LEFT && onTimerPage)
        {
          lv_scr_load_anim(AppWeather_GetScreen(), LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
          onTimerPage = false;
          onWeatherPage = true;
        }
        // LEFT from Weather -> Home
        else if (current_button == BTN_LEFT && onWeatherPage)
        {
          lv_scr_load_anim(home_screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 300, 0, false);
          onWeatherPage = false;
        }

        // ========== TIMER CONTROLS ==========
        if (onTimerPage)
        {
          if (current_button == BTN_CENTER)
          {
            AppTimer_Toggle();
            beep(40);
          }
          else if (current_button == BTN_UP)
          {
            AppTimer_Adjust(1);
          }
          else if (current_button == BTN_DOWN)
          {
            AppTimer_Adjust(-1);
          }
        }
      }
    }

    last_button = current_button;
  }

  // ========== UPDATES ==========
  if (onSnakePage)
  {
    AppSnake_Update();
  }
  else if (onBreakoutPage)
  {
    AppBreakout_Update();
  }
  else
  {
    // Weather update
    static unsigned long lastWeatherUpdate = 0;
    if (millis() - lastWeatherUpdate > 1800000 || lastWeatherUpdate == 0)
    {
      if (WiFi.status() == WL_CONNECTED)
      {
        AppWeather_Update();
        lastWeatherUpdate = millis();
      }
    }

    AppTimer_Update();
  }

  delay(10);
}
// ```

// **Key fixes:**
// 1. ✅ **Removed duplicate** `else if (current_button == BTN_RIGHT)` in Snake menu
// 2. ✅ **Fixed Snake menu navigation**: LEFT → Breakout, RIGHT → Home
// 3. ✅ **Fixed Snake game over navigation**: LEFT → Breakout, RIGHT → Home
// 4. ✅ **Fixed Breakout navigation**: RIGHT → Snake, LEFT → edge (beep)
// 5. ✅ **Updated time check** to exclude both games

// **Navigation flow:**
// ```
// Breakout ← Snake ← Home → Weather → Timer
// (edge)