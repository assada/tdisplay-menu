#include <Arduino.h>

#include <TFT_eSPI.h>
#include <SPI.h>
#include "WiFi.h"
#include <Wire.h>
#include <Button2.h>
#include "bmp.h"
#include "menu.h"

#include "Sreens/InfoScreen.cpp"
#include "Sreens/StarsScreen.cpp"

#include <vector>

#ifndef TFT_DISPOFF
#define TFT_DISPOFF 0x28
#endif

#ifndef TFT_SLPIN
#define TFT_SLPIN   0x10
#endif

TFT_eSPI tft = TFT_eSPI(135, 240);
Button2 btnUp;
Button2 btnDown;

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

InfoScreen infoScreen = InfoScreen(&tft, &menu, &btnUp, &btnDown);
StarsScreen starsScreen = StarsScreen(&tft, &menu, &btnUp, &btnDown);

std::vector<MenuItem> menuItems = {
    MenuItem("Info", {}, infoScreen.action),
    MenuItem("Stars", {}, starsScreen.action),
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
    tft.setTextSize(1);

    infoScreen.initScreen();

    menu.setItems(menuItems);

    if (TFT_BL > 0) {
        pinMode(TFT_BL, OUTPUT);
        digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
    }

    tft.setSwapBytes(true);
    tft.pushImage(0, 0, 240, 135, ttgo);
    espDelay(500);

    tft.setRotation(1);

    menu.buttonsInit(&btnUp, &btnDown);

    menu.redrawMenuItems();
}

void loop() {
    buttonsLoop();
}
