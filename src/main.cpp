#include <Arduino.h>

#include <TFT_eSPI.h>
#include <SPI.h>
#include "WiFi.h"
#include <Wire.h>
#include <Button2.h>
#include "bmp.h"
#include "menu.h"
#include "utils.h"
#include "colors.h"
#include "stars.h"

#include "Sreens/InfoScreen.cpp"

#include <vector>

#ifndef TFT_DISPOFF
#define TFT_DISPOFF 0x28
#endif

#ifndef TFT_SLPIN
#define TFT_SLPIN   0x10
#endif

#define BUTTON_1        35
#define BUTTON_2        0

TFT_eSPI tft = TFT_eSPI(135, 240);
Button2 btnUp;
Button2 btnDown;
bool blockedButton = false;

uint32_t freeHaep();

void buttonsInit();
void reInitButtons();

void starsScreenTask(void* pvParameters);
TaskHandle_t starsTask;

InfoScreen infoScreen = InfoScreen();

MenuAction defaultAction = [](MenuItem& item) { // default action
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);

    String val = "";
    if (item.parameters.find("value") != item.parameters.end()) {
        val = String(": ") + item.parameters["value"];
    }
    tft.drawString(item.title + val, tft.width() / 2, tft.height() / 2);

    return true;
};

Menu menu = Menu(tft);

std::vector<MenuItem> menuItems = {
    MenuItem("Info", {}, infoScreen.action),
    MenuItem("Stars", {}, [](MenuItem&item) {
        tft.fillScreen(MENU_BACKGROUND_COLOR);
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
            tft.fillScreen(MENU_BACKGROUND_COLOR);
            buttonsInit();
            menu.redrawMenuItems();
        });

        return false;
    }),
    MenuItem("Test", {{"flashTime", "500"}}, defaultAction),
    MenuItem("Run", {{"flashTime", "3000"}}, defaultAction),
    MenuItem("Sub-Menu", {}, defaultAction),
    MenuItem("Menu 3", {}, defaultAction),
    MenuItem("Maro", {{"value", "OFF"}, {"flashTime", "100"}}, [](MenuItem&item) {
        tft.fillScreen(TFT_BLACK);
        tft.setTextColor(TFT_WHITE, TFT_BLACK);
        tft.setTextDatum(MC_DATUM);
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

// buttons
void reInitButtons() {
    btnUp.reset();
    btnDown.reset();
    btnUp.begin(BUTTON_1);
    btnDown.begin(BUTTON_2);
    btnUp.setLongClickTime(700);
    btnDown.setLongClickTime(700);
}

void buttonsInit() {
    reInitButtons();

    btnUp.setLongClickDetectedHandler([](Button2&b) {
        blockedButton = true;
        Serial.println("Select Menu Item");
        bool flash = menu.items[menu.selectedMenuItem].onSelect(menu.items[menu.selectedMenuItem]);
        unsigned int delay = 1000;

        if(menu.items[menu.selectedMenuItem].parameters.find("flashTime") != menu.items[menu.selectedMenuItem].parameters.end()) {
            String flashTime = menu.items[menu.selectedMenuItem].parameters["flashTime"];
            delay = flashTime.toInt();
        }

        if (flash != false) {
            espDelay(delay);

            menu.redrawMenuItems();
        }
    });

    btnUp.setReleasedHandler([](Button2&b) {
        if (blockedButton == true) {
            blockedButton = false;
            return;
        }
        menu.up();
    });

    btnDown.setPressedHandler([](Button2&b) {
        menu.down();
    });
}

void buttonsLoop() {
    btnUp.loop();
    btnDown.loop();
}

// buttons end

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
    tft.setTextSize(1);

    infoScreen.initScreen(&tft);

    menu.setItems(menuItems);

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

    menu.redrawMenuItems();
}

void loop() {
    buttonsLoop();
}
