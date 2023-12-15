#pragma once
#include <Arduino.h>
#include <map>
#include <utility>
#include <vector>
#include <TFT_eSPI.h>
#include "colors.h"
#include <Button2.h>
#include "utils.h"
#include "pins.h"

struct MenuItem;

/**
 * \brief return true if need to redraw menu after action
 */
using MenuAction = std::function<bool(MenuItem&)>;

struct MenuItem {
    String title;
    std::map<String, String> parameters;
    MenuAction onSelect;

    explicit MenuItem(
        String title, std::map<String, String> params = {},
        const MenuAction&onSelectFunc = nullptr
    )
        : title(std::move(title)), parameters(std::move(params)) {
        if (onSelectFunc != nullptr) {
            onSelect = onSelectFunc;
        }
        else {
            onSelect = [](MenuItem&item) {
                return true;
            };
        }
    }
};

struct Menu {
    std::vector<MenuItem> items;
    unsigned int currentMenuPage;
    int selectedMenuItem;
    unsigned int maxItemsPerScreen;
    TFT_eSPI* tft;
    bool blockedButton = false;
    String title;

    explicit Menu(TFT_eSPI* tftDisplay,
                  String title = "Menu",
                  unsigned int currentPage = 0,
                  int selectedItem = 0,
                  unsigned int maxItemsScreen = 5)
        : title(std::move(title)),
          currentMenuPage(currentPage),
          selectedMenuItem(selectedItem),
          maxItemsPerScreen(maxItemsScreen),
          tft(tftDisplay) {
    }

    void setItems(const std::vector<MenuItem>&menuItems) {
        this->items = menuItems;
    }

    void up() {
        this->selectedMenuItem--;
        if (this->selectedMenuItem < 0) {
            this->selectedMenuItem = this->items.size() - 1;
        }
        this->currentMenuPage = this->selectedMenuItem / this->maxItemsPerScreen;
        this->redrawMenuItems();
    }

    void down() {
        this->selectedMenuItem++;
        if (this->selectedMenuItem >= this->items.size()) {
            this->selectedMenuItem = 0;
            this->currentMenuPage = 0;
        }
        else {
            this->currentMenuPage = this->selectedMenuItem / this->maxItemsPerScreen;
        }
        this->redrawMenuItems();
    }

    void redrawMenuItems() {
        //Draw Header
        tft->fillScreen(MENU_BACKGROUND_COLOR);
        tft->fillRect(0, 0, 240, 24, MENU_HEADER_BACKGROUND);
        tft->setTextColor(MENU_HEADER_TEXT_COLOR, MENU_HEADER_BACKGROUND);
        tft->setTextDatum(TC_DATUM);
        tft->setTextSize(2);
        tft->drawString(this->title + ": " + String(this->selectedMenuItem + 1) + "/" + this->items.size(), 120, 5);
        //End Header

        //Draw Menu Items
        tft->setTextColor(MENU_ITEM_TEXT_COLOR, MENU_BACKGROUND_COLOR);
        tft->setTextDatum(TL_DATUM);
        for (unsigned int i = this->currentMenuPage * this->maxItemsPerScreen;
             i < min((this->currentMenuPage + 1) * this->maxItemsPerScreen, this->items.size()); i++) {
            if (i == this->selectedMenuItem) {
                tft->fillRect(0, 26 + (i % this->maxItemsPerScreen) * 20, 240, 20, MENU_ITEM_ACTIVE_BACKGROUND);
                tft->setTextColor(MENU_ITEM_TEXT_COLOR, MENU_ITEM_ACTIVE_BACKGROUND);
            }
            else {
                tft->setTextColor(MENU_ITEM_TEXT_COLOR, MENU_BACKGROUND_COLOR);
            }

            String val = "";

            if (this->items[i].parameters.find("value") != this->items[i].parameters.end()) {
                val = ": " + this->items[i].parameters["value"];
            }
            tft->setTextColor(MENU_ITEM_NUMBER_TEXT_COLOR);
            int x = 0;
            if (i + 1 > 9) {
                x = 10;
            }
            tft->drawString(String(i + 1) + ". ", 5, 30 + (i % this->maxItemsPerScreen) * 20);
            tft->setTextColor(MENU_ITEM_TEXT_COLOR); //TODO: color from parameters
            tft->drawString(this->items[i].title + val, 35 + x, 30 + (i % this->maxItemsPerScreen) * 20);
        }
    }

    static void reInitButtons(Button2* btnUp, Button2* btnDown) {
        btnUp->reset();
        btnDown->reset();
        btnUp->begin(BUTTON_1);
        btnDown->begin(BUTTON_2);
        btnUp->setLongClickTime(700);
        btnUp->setDebounceTime(50);
        btnDown->setLongClickTime(700);
        btnDown->setDebounceTime(50);
    }

    void buttonsInit(Button2* btnUp, Button2* btnDown) {
        Serial.println("Buttons Init: " + this->title);
        reInitButtons(btnUp, btnDown);

        btnUp->setLongClickDetectedHandler([this](Button2&b) {
            this->blockedButton = true;
            Serial.println("Select Menu Item: " + this->title);
            bool flash = this->items[this->selectedMenuItem].onSelect(this->items[this->selectedMenuItem]);
            unsigned int delay = 100;
            int delayTime = 0;

            if (this->items[this->selectedMenuItem].parameters.find("flashTime") != this->items[this->selectedMenuItem].
                parameters.end()) {
                const String flashTime = this->items[this->selectedMenuItem].parameters["flashTime"];
                delay = flashTime.toInt();
            }

            if (this->items[this->selectedMenuItem].parameters.find("delayTime") != this->items[this->selectedMenuItem].
                parameters.end()) {
                delayTime = this->items[this->selectedMenuItem].parameters["delayTime"].toInt();
            }

            if (flash != false) {
                espDelay(delay);

                this->redrawMenuItems();
            }

            if (delayTime > 0) {
                espDelay(delayTime);
            }
        });

        btnUp->setReleasedHandler([this](Button2&b) {
            Serial.println("Buttons UP: " + this->title);
            if (this->blockedButton == true) {
                this->blockedButton = false;
                return;
            }
            this->up();
        });

        btnDown->setPressedHandler([this](Button2&b) {
            Serial.println("Buttons DOWN: " + this->title);
            this->down();
        });
    }
};
