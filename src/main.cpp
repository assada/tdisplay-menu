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
#include "Sreens/WiFiScannerScreen.cpp"

#include <vector>

#define TFT_BACKLIGHT_PIN 4

#ifndef TFT_DISPOFF
#define TFT_DISPOFF 0x28
#endif

#ifndef TFT_SLPIN
#define TFT_SLPIN   0x10
#endif

// Setting PWM properties, do not change this!
const int pwmFreq = 5000;
const int pwmResolution = 8;
const int pwmLedChannelTFT = 0;

int ledBacklight = 100;

const uint32_t NVM_Offset = 0x290000;

template<typename T>
void FlashWrite(uint32_t address, const T&value) {
    ESP.flashEraseSector((NVM_Offset + address) / 4096);
    ESP.flashWrite(NVM_Offset + address, (uint32_t *)&value, sizeof(value));
}

template<typename T>
void FlashRead(uint32_t address, T&value) {
    ESP.flashRead(NVM_Offset + address, (uint32_t *)&value, sizeof(value));
}

TFT_eSPI tft = TFT_eSPI(135, 240);
Button2 btnUp;
Button2 btnDown;

MenuAction defaultAction = [](MenuItem&item) {
    // default action
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

Menu menu = Menu(&tft);

InfoScreen infoScreen = InfoScreen(&tft, &menu, &btnUp, &btnDown);
StarsScreen starsScreen = StarsScreen(&tft, &menu, &btnUp, &btnDown);
WiFiScannerScreen wiFiScannerScreen = WiFiScannerScreen(&tft, &menu, &btnUp, &btnDown);

std::vector<MenuItem> menuItems = {
    MenuItem("Info", {}, infoScreen.action),
    MenuItem("Stars", {}, starsScreen.action),
    MenuItem("WiFi Scanner", {}, wiFiScannerScreen.action),
    MenuItem("Nothing", {{"flashTime", "500"}}, defaultAction),
    MenuItem("Backlight", {{"value", String(ledBacklight)}}, [](MenuItem&item) {
        ledBacklight += 50;
        if (ledBacklight > 255) {
            ledBacklight = 50;
        }
        FlashWrite<int>(0, ledBacklight);
        item.parameters["value"] = String(ledBacklight);
        menu.redrawMenuItems();

        return false;
    }),
    MenuItem("Nothing", {}, defaultAction),
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
    while (!Serial);
    Serial.flush();
    uint32_t seed = esp_random();
    Serial.printf("Setting random seed %u\n", seed);

    Serial.printf("Total heap: %d\n", ESP.getHeapSize());
    Serial.printf("Free heap: %d\n", ESP.getFreeHeap());
    Serial.printf("Total PSRAM: %d\n", ESP.getPsramSize());
    Serial.printf("Free PSRAM: %d\n", ESP.getFreePsram());


    FlashRead<int>(0, ledBacklight);
    Serial.printf("Read ledBacklight: %d\n", ledBacklight);
    if (ledBacklight == -1) {
        ledBacklight = 100;
    }
    menuItems.at(4).parameters["value"] = String(ledBacklight); //=(((

    pinMode(TFT_BACKLIGHT_PIN, OUTPUT);
    ledcSetup(pwmLedChannelTFT, pwmFreq, pwmResolution);
    ledcAttachPin(TFT_BACKLIGHT_PIN, pwmLedChannelTFT);
    ledcWrite(pwmLedChannelTFT, ledBacklight);

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

    tft.setSwapBytes(true);
    tft.pushImage(0, 0, 240, 135, ttgo);
    espDelay(500);

    tft.setRotation(1);

    menu.buttonsInit(&btnUp, &btnDown);

    menu.redrawMenuItems();
}

void loop() {
    buttonsLoop();
    ledcWrite(pwmLedChannelTFT, ledBacklight);
}
