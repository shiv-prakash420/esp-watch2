#include "AppWeather.h"
#include "weather_icons.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

static lv_obj_t *weather_screen;
static lv_obj_t *city_label;

// UI Elements for Data
static lv_obj_t *temp_icon_obj;
static lv_obj_t *temp_val_label;

static lv_obj_t *wind_icon_obj;
static lv_obj_t *wind_val_label;

static lv_obj_t *rain_icon_obj;
static lv_obj_t *rain_val_label;

static lv_obj_t *status_icon_obj;
static lv_obj_t *status_desc_label;

void AppWeather_Init()
{
    weather_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(weather_screen, lv_color_hex(0x000000), 0);

    // 1. City Name
    city_label = lv_label_create(weather_screen);
    lv_label_set_text(city_label, "GREATER NOIDA");
    lv_obj_set_style_text_font(city_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(city_label, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_align(city_label, LV_ALIGN_TOP_MID, 0, 10);

    // 2. Temperature Row
    temp_icon_obj = lv_img_create(weather_screen);
    lv_img_set_src(temp_icon_obj, &temp);
    lv_obj_align(temp_icon_obj, LV_ALIGN_TOP_LEFT, 15, 45);

    temp_val_label = lv_label_create(weather_screen);
    lv_obj_set_style_text_font(temp_val_label, &lv_font_montserrat_24, 0);
    lv_obj_set_style_text_color(temp_val_label, lv_color_hex(0xCCCCCC), 0);
    lv_label_set_text(temp_val_label, "--.-°C");
    lv_obj_align_to(temp_val_label, temp_icon_obj, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    // 3. Wind Speed Row
    wind_icon_obj = lv_img_create(weather_screen);
    lv_img_set_src(wind_icon_obj, &wind);
    lv_obj_align(wind_icon_obj, LV_ALIGN_TOP_LEFT, 20, 90);

    wind_val_label = lv_label_create(weather_screen);
    lv_label_set_text(wind_val_label, "0.0 km/h");
    lv_obj_set_style_text_color(wind_val_label, lv_color_hex(0xCCCCCC), 0);
    lv_obj_align_to(wind_val_label, wind_icon_obj, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    // 4. Precipitation Row
    rain_icon_obj = lv_img_create(weather_screen);
    lv_img_set_src(rain_icon_obj, &rainy);
    lv_obj_align(rain_icon_obj, LV_ALIGN_TOP_LEFT, 20, 130);

    rain_val_label = lv_label_create(weather_screen);
    lv_label_set_text(rain_val_label, "Rain: 0%");
    lv_obj_set_style_text_color(rain_val_label, lv_color_hex(0xCCCCCC), 0);
    lv_obj_align_to(rain_val_label, rain_icon_obj, LV_ALIGN_OUT_RIGHT_MID, 10, 0);

    // 5. Overall Weather Status (Bottom)
    status_icon_obj = lv_img_create(weather_screen);
    lv_img_set_src(status_icon_obj, &clear); // Default
    lv_obj_align(status_icon_obj, LV_ALIGN_BOTTOM_LEFT, 20, -30);

    status_desc_label = lv_label_create(weather_screen);
    lv_label_set_text(status_desc_label, "Clear Sky");
    lv_obj_set_style_text_color(status_desc_label, lv_palette_main(LV_PALETTE_AMBER), 0);
    lv_obj_align_to(status_desc_label, status_icon_obj, LV_ALIGN_OUT_RIGHT_MID, 10, 0);
}

lv_obj_t *AppWeather_GetScreen() { return weather_screen; }

void AppWeather_Update() {
    if (weather_screen == NULL || temp_val_label == NULL) return;
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected!");
        return;
    }

    Serial.println("Fetching weather data...");
    HTTPClient http;
    String url = "http://api.open-meteo.com/v1/forecast?latitude=28.47&longitude=77.50&current=temperature_2m,weather_code,wind_speed_10m&hourly=precipitation_probability&forecast_days=1";
    
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println("Got API response");
        
        DynamicJsonDocument doc(2048); 
        deserializeJson(doc, payload);

        float t = doc["current"]["temperature_2m"];
        float w = doc["current"]["wind_speed_10m"];
        int code = doc["current"]["weather_code"];
        int rain_prob = doc["hourly"]["precipitation_probability"][0];

        Serial.printf("Raw data: %.1f°C, %.1f km/h, %d%% rain\n", t, w, rain_prob);

        // CRITICAL FIX: Use static buffers for text
        static char temp_buf[16];
        static char wind_buf[16];
        static char rain_buf[16];
        
        sprintf(temp_buf, "%.1f°C", t);
        sprintf(wind_buf, "%.1f km/h", w);
        sprintf(rain_buf, "Rain: %d%%", rain_prob);
        
        lv_label_set_text(temp_val_label, temp_buf);
        lv_label_set_text(wind_val_label, wind_buf);
        lv_label_set_text(rain_val_label, rain_buf);

        if (code == 0) lv_label_set_text(status_desc_label, "Clear Sky");
        else if (code <= 3) lv_label_set_text(status_desc_label, "Cloudy");
        else if (code >= 95) lv_label_set_text(status_desc_label, "Stormy");
        else lv_label_set_text(status_desc_label, "Rainy");

        // Force refresh
        lv_obj_invalidate(temp_val_label);
        lv_obj_invalidate(wind_val_label);
        lv_obj_invalidate(rain_val_label);
        lv_obj_invalidate(status_desc_label);
        
        Serial.println("Labels updated!");
    } else {
        Serial.printf("HTTP Error: %d\n", httpCode);
    }
    http.end();
}