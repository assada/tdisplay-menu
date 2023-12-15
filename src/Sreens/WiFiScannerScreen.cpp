#include <TFT_eSPI.h>
#include "menu.h"
#include <Button2.h>
#include "colors.h"
#include "WiFi.h"
#include "menu.h"

class WiFiScannerScreen {
public:
    explicit WiFiScannerScreen(TFT_eSPI* tftDisplay, Menu* mainMenu, Button2* btnUp, Button2* btnDown): wifiMenu(
        tftDisplay, "WiFi") {
        this->tft = tftDisplay;
        this->mainMenu = mainMenu;
        this->btnUp = btnUp;
        this->btnDown = btnDown;
    }

    MenuAction action = [this](MenuItem&item) {
        this->tft->fillScreen(MENU_BACKGROUND_COLOR);
        this->mainMenu->reInitButtons(this->btnUp, this->btnDown); //reset main menu buttons
        /*this->btnDown->setLongClickDetectedHandler([this](Button2&b) {
            this->tft->fillScreen(MENU_BACKGROUND_COLOR);
            this->mainMenu->buttonsInit(this->btnUp, this->btnDown);
            this->mainMenu->redrawMenuItems();
        });*/

        this->wifiScan();

        return false;
    };

private:
    TFT_eSPI* tft;
    Menu* mainMenu;
    Button2* btnDown;
    Button2* btnUp;
    char buff[512];

    Menu wifiMenu;

    void wifiScan() {
        MenuAction backAction = [this](MenuItem&item) {
            this->wifiMenu.reInitButtons(this->btnUp, this->btnDown);
            this->mainMenu->buttonsInit(this->btnUp, this->btnDown); //enable main menu buttons
            this->mainMenu->redrawMenuItems();

            return true;
        };

        std::vector<MenuItem> menuItems = {
            MenuItem("<< Back", {{"delayTime", "500"}}, backAction)
        };

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
        }
        else {
            this->tft->setTextDatum(TL_DATUM);
            this->tft->setCursor(0, 0);
            for (int i = 0; i < n; ++i) {
                sprintf(this->buff,
                        "%s(%d)",
                        WiFi.SSID(i).c_str(),
                        WiFi.RSSI(i));
                menuItems.push_back(MenuItem(buff, {}, [](MenuItem&item) { //connect to selected AP and return to main menu
                    return false; //do nothing
                }));
            }
            this->wifiMenu.setItems(menuItems);
            this->wifiMenu.buttonsInit(this->btnUp, this->btnDown); //start wifi menu buttons
            this->wifiMenu.redrawMenuItems();
        }
        WiFi.mode(WIFI_OFF);
    }
};
