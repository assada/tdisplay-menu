#include <TFT_eSPI.h>
#include "menu.h"
#include "esp_adc_cal.h"
#include <Button2.h>
#include "colors.h"

#define ADC_EN          14
#define ADC_PIN         34

class InfoScreen {
public:
    explicit InfoScreen(TFT_eSPI *tftDisplay, Menu *menu, Button2 *btnUp, Button2 *btnDown) {
        this->tft = tftDisplay;
        this->menu = menu;
        this->btnUp = btnUp;
        this->btnDown = btnDown;
    }

    MenuAction action = [this](MenuItem &item) {
        Serial.println("InfoScreen action");
        xTaskCreatePinnedToCore(
            this->infoScreenTask,
            "infoScreen",
            10000,
            this,
            2,
            &this->infoTask,
            0
        );
        this->menu->reInitButtons(this->btnUp, this->btnDown);

        this->btnDown->setLongClickDetectedHandler([this](Button2&b) {
            vTaskDelete(this->infoTask);
            this->tft->fillScreen(MENU_BACKGROUND_COLOR);
            this->menu->buttonsInit(this->btnUp, this->btnDown);
            this->menu->redrawMenuItems();
        });

        return false;
    };

    static void infoScreenTask(void* pvParameters) {
        InfoScreen *l_pThis = (InfoScreen *) pvParameters;

        static uint64_t timeStamp = 0;
        l_pThis->tft->fillScreen(TFT_BLACK);
        l_pThis->tft->setTextDatum(MC_DATUM);

        for (;;) {
            timeStamp = millis();
            uint16_t v = analogRead(ADC_PIN);
            float battery_voltage = ((float)v / 4095.0) * 2.0 * 3.3 * (l_pThis->vref / 1000.0);
            String voltage = "Voltage: " + String(battery_voltage) + "V";
            String freeHeap = "Free heap: " + static_cast<String>(ESP.getFreeHeap());
            l_pThis->tft->setTextSize(2);
            l_pThis->tft->setTextColor(TFT_WHITE, TFT_BLACK);
            l_pThis->tft->drawString(voltage, l_pThis->tft->width() / 2, l_pThis->tft->height() / 2);
            l_pThis->tft->drawString(freeHeap, l_pThis->tft->width() / 2, l_pThis->tft->height() / 2 + 20);
            l_pThis->tft->drawString(String(timeStamp), l_pThis->tft->width() / 2, l_pThis->tft->height() / 2 + 40);
            l_pThis->tft->setTextSize(1);
            l_pThis->tft->drawString("Hold Down to exit", l_pThis->tft->width() / 2, l_pThis->tft->height() / 2 + 60);
            vTaskDelay(10 / portTICK_RATE_MS);
        }
    }

    void initScreen() {
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
            this->vref = adc_chars.vref;
        }
        else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
            Serial.printf("Two Point --> coeff_a:%umV coeff_b:%umV\n", adc_chars.coeff_a, adc_chars.coeff_b);
        }
        else {
            Serial.println("Default Vref: 1100mV");
        }
    }
private:
    TFT_eSPI *tft;
    Menu *menu;
    Button2 *btnDown;
    Button2 *btnUp;

    TaskHandle_t infoTask;
    unsigned int vref = 1100;
};