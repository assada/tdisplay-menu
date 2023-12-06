#include <TFT_eSPI.h>
#include "menu.h"
#include <Button2.h>
#include "colors.h"
#define NSTARS 1024

class StarsScreen {
public:
    explicit StarsScreen(TFT_eSPI *tftDisplay, Menu *menu, Button2 *btnUp, Button2 *btnDown) {
        this->tft = tftDisplay;
        this->menu = menu;
        this->btnUp = btnUp;
        this->btnDown = btnDown;
    }

    uint8_t rng() {
        zx++;
        za = (za ^ zc ^ zx);
        zb = (zb + za);
        zc = ((zc + (zb >> 1)) ^ za);
        return zc;
    }

    MenuAction action = [this](MenuItem &item) {
        this->tft->fillScreen(MENU_BACKGROUND_COLOR);
        this->za = random(256);
        this->zb = random(256);
        this->zc = random(256);
        this->zx = random(256);
        xTaskCreate(
            this->starsScreenTask,
            "starsScreen",
            10000,
            this,
            5,
            &this->starsTask
        );
        this->menu->reInitButtons(this->btnUp, this->btnDown);

        this->btnDown->setLongClickDetectedHandler([this](Button2&b) {
            vTaskDelete(this->starsTask);
            this->tft->fillScreen(MENU_BACKGROUND_COLOR);
            this->menu->buttonsInit(this->btnUp, this->btnDown);
            this->menu->redrawMenuItems();
        });
        return false;
    };

    static void starsScreenTask(void* pvParameters) {
        StarsScreen *l_pThis = (StarsScreen *) pvParameters;
        for (;;) {
            uint8_t spawnDepthVariation = 255;

            for (int i = 0; i < NSTARS; ++i) {
                if (l_pThis->sz[i] <= 1) {
                    l_pThis->sx[i] = 160 - 120 + l_pThis->rng();
                    l_pThis->sy[i] = l_pThis->rng();
                    l_pThis->sz[i] = spawnDepthVariation--;
                }
                else {
                    int old_screen_x = ((int)l_pThis->sx[i] - 160) * 256 / l_pThis->sz[i] + 160;
                    int old_screen_y = ((int)l_pThis->sy[i] - 120) * 256 / l_pThis->sz[i] + 120;
                    l_pThis->tft->drawPixel(old_screen_x, old_screen_y,TFT_BLACK);

                    l_pThis->sz[i] -= 2;
                    if (l_pThis->sz[i] > 1) {
                        int screen_x = ((int)l_pThis->sx[i] - 160) * 256 / l_pThis->sz[i] + 160;
                        int screen_y = ((int)l_pThis->sy[i] - 120) * 256 / l_pThis->sz[i] + 120;

                        if (screen_x >= 0 && screen_y >= 0 && screen_x < 240 && screen_y < 135) {
                            uint8_t r, g, b;
                            r = g = b = 255 - l_pThis->sz[i];
                            l_pThis->tft->drawPixel(screen_x, screen_y, l_pThis->tft->color565(r, g, b));
                        }
                        else
                            l_pThis->sz[i] = 0;
                    }
                }
            }

            vTaskDelay(10 / portTICK_RATE_MS);
        }
    }
private:
    TFT_eSPI *tft;
    Menu *menu;
    Button2 *btnDown;
    Button2 *btnUp;

    uint8_t sx[NSTARS] = {};
    uint8_t sy[NSTARS] = {};
    uint8_t sz[NSTARS] = {};
    uint8_t za, zb, zc, zx;

    TaskHandle_t starsTask;
};