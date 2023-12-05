#include <Arduino.h>

#include <TFT_eSPI.h>
#include <SPI.h>
#include "WiFi.h"
#include <Wire.h>
#include <Button2.h>
#include "esp_adc_cal.h"
#include "bmp.h"

#include <vector>
#include <map>
#include <string>

#ifndef TFT_DISPOFF
#define TFT_DISPOFF 0x28
#endif

#ifndef TFT_SLPIN
#define TFT_SLPIN   0x10
#endif

#define ADC_EN          14
#define ADC_PIN         34
#define BUTTON_1        35
#define BUTTON_2        0

TFT_eSPI tft = TFT_eSPI(135, 240);
Button2 btnUp;
Button2 btnDown;

char buff[512];
int vref = 1100;

uint32_t freeHaep();

int currentMenuPage = 0;
int selectedMenuItem = 0; // current selected menu item

const int NUM_ITEMS = 7;
const int MAX_ITEM_LENGTH = 20;
const int MAX_ITEMS_PER_SCREEN = 5; // maximum number of items per screen

bool blockedButton = false;

void buttonsInit();

void showVoltage(void* pvParameters);

void reInitButtons();

void redrawMenuItems();

void starsScreenTask(void* pvParameters);

uint8_t za, zb, zc, zx;

#define NSTARS 1024
uint8_t sx[NSTARS] = {};
uint8_t sy[NSTARS] = {};
uint8_t sz[NSTARS] = {};

uint8_t rng() {
    zx++;
    za = (za ^ zc ^ zx);
    zb = (zb + za);
    zc = ((zc + (zb >> 1)) ^ za);
    return zc;
}

TaskHandle_t Task1;
TaskHandle_t starsTask;

struct MenuItem;

using MenuAction = std::function<bool(MenuItem&)>; //return true if need to redraw menu after action

struct MenuItem {
    String title;
    std::map<String, String> parameters;
    MenuAction onSelect;

    // Конструктор
    MenuItem(String t, std::map<String, String> params = {},
             MenuAction onSelectFunc = nullptr)
        : title(t), parameters(params) {
        if (onSelectFunc != nullptr) {
            onSelect = onSelectFunc;
        } else {
            onSelect = [](MenuItem& item) { // default action
                String val = "";
                if (item.parameters.find("value") != item.parameters.end()) {
                    val = String(": ") + item.parameters["value"];
                }
                tft.drawString(item.title + val, tft.width() / 2, tft.height() / 2);

                return true;
            };
        }
    }
};

std::vector<MenuItem> menu = {
    MenuItem("Info", {}, [](MenuItem&item) {
        xTaskCreate(
            showVoltage,
            "infoScreen",
            10000,
            nullptr,
            5,
            &Task1
        );
        reInitButtons();

        btnDown.setLongClickDetectedHandler([](Button2&b) {
            vTaskDelete(Task1);
            tft.fillScreen(TFT_BLACK);
            buttonsInit();
            redrawMenuItems();
        });

        return false;
    }),
    MenuItem("Stars", {}, [](MenuItem&item) {
        za = random(256);
        zb = random(256);
        zc = random(256);
        zx = random(256);
        xTaskCreate(
            starsScreenTask,
            "starsScreen",
            10000,
            nullptr,
            5,
            &starsTask
        );
        reInitButtons();

        btnDown.setLongClickDetectedHandler([](Button2&b) {
            vTaskDelete(starsTask);
            tft.fillScreen(TFT_BLACK);
            buttonsInit();
            redrawMenuItems();
        });

        return false;
    }),
    MenuItem("Test", {{"flashTime", "500"}}),
    MenuItem("Run", {{"flashTime", "3000"}}),
    MenuItem("Sub-Menu", {{"flashTime", "1000"}}),
    MenuItem("Menu 3", {{"flashTime", "1000"}}),
    MenuItem("Maro", {{"value", "OFF"}, {"flashTime", "100"}}, [](MenuItem&item) {
        if (item.parameters["value"] == "OFF") {
            item.parameters["value"] = "ON";
        }
        else {
            item.parameters["value"] = "OFF";
        }
        tft.drawString(item.title + ": " + item.parameters["value"], tft.width() / 2, tft.height() / 2);

        return true;
    })
};

void espDelay(int ms) {
    esp_sleep_enable_timer_wakeup(ms * 1000);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
    esp_light_sleep_start();
}

void showVoltage(void* pvParameters) {
    static uint64_t timeStamp = 0;
    for (;;) {
        timeStamp = millis();
        uint16_t v = analogRead(ADC_PIN);
        float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);
        String voltage = "Voltage: " + String(battery_voltage) + "V";
        String freeHeap = "Free heap: " + static_cast<String>(ESP.getFreeHeap());
        tft.fillScreen(TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        tft.setTextSize(2);
        tft.drawString(voltage, tft.width() / 2, tft.height() / 2);
        tft.drawString(freeHeap, tft.width() / 2, tft.height() / 2 + 20);
        tft.drawString(String(timeStamp), tft.width() / 2, tft.height() / 2 + 40);
        tft.setTextSize(1);
        tft.drawString("Hold Down to exit", tft.width() / 2, tft.height() / 2 + 60);
        vTaskDelay(200 / portTICK_RATE_MS);
    }
}

void reInitButtons() {
    btnUp.reset();
    btnDown.reset();
    btnUp.begin(BUTTON_1);
    btnDown.begin(BUTTON_2);
    btnUp.setLongClickTime(700);
    btnDown.setLongClickTime(700);
}

void redrawMenuItems() {
    //Draw Header
    tft.fillScreen(TFT_BLACK);
    tft.fillRect(0, 0, 240, 24, TFT_MAROON);
    tft.setTextColor(TFT_WHITE, TFT_MAROON);
    tft.setTextDatum(TC_DATUM);
    tft.setTextSize(2);
    tft.drawString("Menu: " + String(selectedMenuItem + 1) + "/" + String(NUM_ITEMS), 120, 5);
    //End Header

    //Draw Menu Items
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(TL_DATUM);
    for (int i = currentMenuPage * MAX_ITEMS_PER_SCREEN;
         i < min((currentMenuPage + 1) * MAX_ITEMS_PER_SCREEN, NUM_ITEMS); i++) {
        if (i == selectedMenuItem) {
            tft.fillRect(0, 26 + (i % MAX_ITEMS_PER_SCREEN) * 20, 240, 20, TFT_MAROON);
            tft.setTextColor(TFT_WHITE, TFT_MAROON);
        }
        else {
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
        }

        String val = "";

        if (menu[i].parameters.find("value") != menu[i].parameters.end()) {
            val = ": " + menu[i].parameters["value"];
        }
        tft.setTextColor(TFT_LIGHTGREY);
        tft.drawString(String(i) + ". ", 5, 30 + (i % MAX_ITEMS_PER_SCREEN) * 20);
        tft.setTextColor(TFT_WHITE);
        tft.drawString(menu[i].title + val, 35, 30 + (i % MAX_ITEMS_PER_SCREEN) * 20);
    }
}

void starsScreenTask(void* pvParameters) {
    for (;;) {
        uint8_t spawnDepthVariation = 255;

        for (int i = 0; i < NSTARS; ++i) {
            if (sz[i] <= 1) {
                sx[i] = 160 - 120 + rng();
                sy[i] = rng();
                sz[i] = spawnDepthVariation--;
            }
            else {
                int old_screen_x = ((int)sx[i] - 160) * 256 / sz[i] + 160;
                int old_screen_y = ((int)sy[i] - 120) * 256 / sz[i] + 120;

                // This is a faster pixel drawing function for occassions where many single pixels must be drawn
                tft.drawPixel(old_screen_x, old_screen_y,TFT_BLACK);

                sz[i] -= 2;
                if (sz[i] > 1) {
                    int screen_x = ((int)sx[i] - 160) * 256 / sz[i] + 160;
                    int screen_y = ((int)sy[i] - 120) * 256 / sz[i] + 120;

                    if (screen_x >= 0 && screen_y >= 0 && screen_x < 240 && screen_y < 135) {
                        uint8_t r, g, b;
                        r = g = b = 255 - sz[i];
                        tft.drawPixel(screen_x, screen_y, tft.color565(r, g, b));
                    }
                    else
                        sz[i] = 0; // Out of screen, die.
                }
            }
        }

        vTaskDelay(10 / portTICK_RATE_MS);
    }
}

void buttonsInit() {
    reInitButtons();

    btnUp.setLongClickTime(700);
    btnUp.setLongClickDetectedHandler([](Button2&b) {
        blockedButton = true;
        Serial.println("Select Menu Item");
        // selectedMenuItem++;
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
        bool flash = menu[selectedMenuItem].onSelect(menu[selectedMenuItem]);

        int delay = 1000;

        if(menu[selectedMenuItem].parameters.find("flashTime") != menu[selectedMenuItem].parameters.end()) {
            String flashTime = menu[selectedMenuItem].parameters["flashTime"];
            delay = flashTime.toInt();
        }

        if (flash != false) {
            espDelay(delay);

            redrawMenuItems();
        }
    });

    btnUp.setReleasedHandler([](Button2&b) {
        if (blockedButton == true) {
            blockedButton = false;
            return;
        }
        Serial.println("Previous Menu Item");
        selectedMenuItem--;
        if (selectedMenuItem < 0) {
            selectedMenuItem = NUM_ITEMS - 1;
        }
        currentMenuPage = selectedMenuItem / MAX_ITEMS_PER_SCREEN;
        redrawMenuItems();
    });

    btnDown.setPressedHandler([](Button2&b) {
        Serial.println("Next Menu Item");
        selectedMenuItem++;
        if (selectedMenuItem >= NUM_ITEMS) {
            selectedMenuItem = 0;
            currentMenuPage = 0;
        }
        else {
            currentMenuPage = selectedMenuItem / MAX_ITEMS_PER_SCREEN;
        }
        redrawMenuItems();
    });
}

void buttonsLoop() {
    btnUp.loop();
    btnDown.loop();
}

void setup() {
    Serial.begin(115200);
    Serial.println("Start");
    tft.init();
    tft.setRotation(1);
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(2);
    tft.setTextColor(TFT_WHITE);
    tft.setCursor(0, 0);
    tft.setTextDatum(MC_DATUM);
    tft.setTextSize(1.8);

    if (TFT_BL > 0) {
        // TFT_BL has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
        pinMode(TFT_BL, OUTPUT); // Set backlight pin to output mode
        digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
        // Turn backlight on. TFT_BACKLIGHT_ON has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
    }

    tft.setSwapBytes(true);
    tft.pushImage(0, 0, 240, 135, ttgo);
    espDelay(500);

    tft.setRotation(1);

    buttonsInit();

    esp_adc_cal_characteristics_t adc_chars;
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize( //Check type of calibration value used to characterize ADC
        (adc_unit_t)ADC_UNIT_1,
        (adc_atten_t)ADC1_CHANNEL_6,
        (adc_bits_width_t)ADC_WIDTH_BIT_12,
        1100,
        &adc_chars
    );

    if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        Serial.printf("eFuse Vref:%u mV", adc_chars.vref);
        vref = adc_chars.vref;
    }
    else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        Serial.printf("Two Point --> coeff_a:%umV coeff_b:%umV\n", adc_chars.coeff_a, adc_chars.coeff_b);
    }
    else {
        Serial.println("Default Vref: 1100mV");
    }

    redrawMenuItems();
}

void loop() {
    buttonsLoop();
}
