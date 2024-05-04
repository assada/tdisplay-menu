#include <TFT_eSPI.h>
#include "menu.h"
#include <Button2.h>
#include "colors.h"
#include <EMA.h>

#define ANALOG_PIN 38

         //initialization of EMA S

class PotTestScreen {
public:
    explicit PotTestScreen(TFT_eSPI *tftDisplay, Menu *menu, Button2 *btnUp, Button2 *btnDown) {
        this->tft = tftDisplay;
        this->menu = menu;
        this->btnUp = btnUp;
        this->btnDown = btnDown;
    }

    MenuAction action = [this](MenuItem &item) {
        this->tft->fillScreen(MENU_BACKGROUND_COLOR);
        xTaskCreate(
            this->potMainTask,
            "starsScreen",
            10000,
            this,
            5,
            &this->potTask
        );
        this->menu->reInitButtons(this->btnUp, this->btnDown);

        this->btnDown->setLongClickDetectedHandler([this](Button2&b) {
            vTaskDelete(this->potTask);
            this->tft->fillScreen(MENU_BACKGROUND_COLOR);
            this->menu->buttonsInit(this->btnUp, this->btnDown);
            this->menu->redrawMenuItems();
        });
        return false;
    };

    static void potMainTask(void* pvParameters) {
        PotTestScreen *l_pThis = (PotTestScreen *) pvParameters;
        int potValue = 0;     //initialization of EMA alpha
        int initPotValue = analogRead(ANALOG_PIN); 

        l_pThis->tft->setTextColor(TFT_WHITE, MENU_BACKGROUND_COLOR);
        l_pThis->tft->setTextSize(2);

        static EMA<3> EMA_filter(initPotValue);

        #define MAX_MENU_ITEMS 6
        int current_menu_item = 1;

        uint16_t potValue_filtered;

        //l_pThis->tft->startWrite();

        for (;;) {
            l_pThis->tft->fillScreen(MENU_BACKGROUND_COLOR);
            potValue = analogRead(ANALOG_PIN);

            potValue_filtered = EMA_filter(potValue);

            int menuItem = (potValue * MAX_MENU_ITEMS) / 4096;

            if (menuItem >= MAX_MENU_ITEMS) {
                menuItem = MAX_MENU_ITEMS - 1;
            }

            menuItem = menuItem + 1;

            l_pThis->tft->drawString("Pot value: ", 10, 10, 2);
            l_pThis->tft->drawNumber(potValue_filtered, 10, 40, 2);

            l_pThis->tft->drawString("Menu: ", 10, 70, 2);
            l_pThis->tft->drawNumber(menuItem, 10, 100, 2);

            //l_pThis->tft->drawRect(10, 40, 120, 70, TFT_BLACK);

            vTaskDelay(10 / portTICK_RATE_MS);
        }
    }
private:
    TFT_eSPI *tft;
    Menu *menu;
    Button2 *btnDown;
    Button2 *btnUp;

    TaskHandle_t potTask;
};