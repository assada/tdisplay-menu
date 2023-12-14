#include <TFT_eSPI.h>
#include "menu.h"
#include <Button2.h>
#include "colors.h"
#include "WiFi.h"

class WiFiScannerScreen {
public:
    explicit WiFiScannerScreen(TFT_eSPI *tftDisplay, Menu *menu, Button2 *btnUp, Button2 *btnDown) {
        this->tft = tftDisplay;
        this->menu = menu;
        this->btnUp = btnUp;
        this->btnDown = btnDown;
    }

    MenuAction action = [this](MenuItem &item) {
        this->tft->fillScreen(MENU_BACKGROUND_COLOR);
        this->menu->reInitButtons(this->btnUp, this->btnDown);

        this->btnDown->setLongClickDetectedHandler([this](Button2&b) {
            this->tft->fillScreen(MENU_BACKGROUND_COLOR);
            this->menu->buttonsInit(this->btnUp, this->btnDown);
            this->menu->redrawMenuItems();
        });

        this->wifiScan();

        return false;
    };


private:
    TFT_eSPI *tft;
    Menu *menu;
    Button2 *btnDown;
    Button2 *btnUp;
    char buff[512];

    void wifiScan()
    {
        this->tft->setTextColor(TFT_GREEN, MENU_BACKGROUND_COLOR);
        this->tft->fillScreen(MENU_BACKGROUND_COLOR);
        this->tft->setTextDatum(MC_DATUM);
        this->tft->setTextSize(2);

        this->tft->drawString("Scanning Network...", this->tft->width() / 2, this->tft->height() / 2);

        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        delay(100);

        this->tft->setTextSize(1);

        int16_t n = WiFi.scanNetworks();
        this->tft->fillScreen(MENU_BACKGROUND_COLOR);
        if (n == 0) {
            this->tft->drawString("no networks found", this->tft->width() / 2, this->tft->height() / 2);
        } else {
            this->tft->setTextDatum(TL_DATUM);
            this->tft->setCursor(0, 0);
            for (int i = 0; i < n; ++i) {
                sprintf(this->buff,
                        "[%d]:%s(%d)",
                        i + 1,
                        WiFi.SSID(i).c_str(),
                        WiFi.RSSI(i));
                this->tft->println(buff);
            }
        }
        WiFi.mode(WIFI_OFF);
    }
};