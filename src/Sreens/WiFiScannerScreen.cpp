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

        this->wifiScan();


        return false;
    };

private:
    TFT_eSPI* tft;
    Menu* mainMenu;
    Button2* btnDown;
    Button2* btnUp;
    char buff[512];

    String ssid;
    TaskHandle_t connectTask;

    Menu wifiMenu;

    static void tryConnectTask (void* pvParameters) {
        WiFiScannerScreen *l_pThis = (WiFiScannerScreen *) pvParameters;

        for(;;) {
            WiFi.mode(WIFI_STA);
            WiFi.begin(l_pThis->ssid.c_str(), "qqqq1111");
            l_pThis->tft->fillScreen(MENU_BACKGROUND_COLOR);
            l_pThis->tft->setTextColor(TFT_WHITE, MENU_BACKGROUND_COLOR);
            l_pThis->tft->setTextDatum(MC_DATUM);
            l_pThis->tft->drawString("Connecting to " + l_pThis->ssid, l_pThis->tft->width() / 2, l_pThis->tft->height() / 2);
            while (WiFi.status() != WL_CONNECTED) {
                delay(500);
                l_pThis->tft->drawString(".", 10, l_pThis->tft->height() / 2);
            }
            l_pThis->tft->fillScreen(MENU_BACKGROUND_COLOR);
            l_pThis->tft->drawString("Connected to " + l_pThis->ssid, l_pThis->tft->width() / 2, l_pThis->tft->height() / 2);
            delay(1000);
            l_pThis->tft->fillScreen(MENU_BACKGROUND_COLOR);
            l_pThis->tft->drawString("IP: " + WiFi.localIP().toString(), l_pThis->tft->width() / 2, l_pThis->tft->height() / 2);
            delay(1000);
            l_pThis->tft->fillScreen(MENU_BACKGROUND_COLOR);
            l_pThis->tft->drawString("Subnet: " + WiFi.subnetMask().toString(), l_pThis->tft->width() / 2, l_pThis->tft->height() / 2);
            delay(1000);
            l_pThis->tft->fillScreen(MENU_BACKGROUND_COLOR);
            l_pThis->tft->drawString("Gateway: " + WiFi.gatewayIP().toString(), l_pThis->tft->width() / 2, l_pThis->tft->height() / 2);
            delay(1000);
            l_pThis->tft->fillScreen(MENU_BACKGROUND_COLOR);
            l_pThis->tft->drawString("DNS: " + WiFi.dnsIP().toString(), l_pThis->tft->width() / 2, l_pThis->tft->height() / 2);
            delay(1000);
            l_pThis->tft->fillScreen(MENU_BACKGROUND_COLOR);
            l_pThis->tft->drawString("MAC: " + WiFi.macAddress(), l_pThis->tft->width() / 2, l_pThis->tft->height() / 2);
            WiFi.disconnect();
            vTaskDelete(l_pThis->connectTask);
            break;
        }
    }

    void wifiScan() {
        std::vector<MenuItem> menuItems = {};

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
                menuItems.push_back(MenuItem(buff, {{"ssid", WiFi.SSID(i).c_str()}}, [this](MenuItem&item) {
                    this->ssid = item.parameters["ssid"];
                    xTaskCreatePinnedToCore(
                        this->tryConnectTask,
                        "infoScreen",
                        10000,
                        this,
                        2,
                        &this->connectTask,
                        0
                    );
                    return false; //do nothing
                }));
            }
            this->wifiMenu.setItems(menuItems);
            this->wifiMenu.buttonsInit(this->btnUp, this->btnDown); //start wifi menu buttons
            this->btnDown->setLongClickDetectedHandler([this](Button2&b) {
                this->wifiMenu.reInitButtons(this->btnUp, this->btnDown);

                this->tft->fillScreen(MENU_BACKGROUND_COLOR);
                this->mainMenu->buttonsInit(this->btnUp, this->btnDown);
                this->mainMenu->redrawMenuItems();
            });
            this->wifiMenu.redrawMenuItems();
        }
        WiFi.mode(WIFI_OFF);
    }
};
