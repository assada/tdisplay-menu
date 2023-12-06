#include <Arduino.h>
#include <map>
#include <map>
#include <utility>
#include <vector>

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
        String t, std::map<String, String> params = {},
        const MenuAction&onSelectFunc = nullptr
    )
        : title(std::move(t)), parameters(std::move(params)) {
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

    explicit Menu(const std::vector<MenuItem>&menuItems,
                  unsigned int currentPage = 0,
                  int selectedItem = 0,
                  unsigned int maxItemsScreen = 5)
        : items(menuItems),
          currentMenuPage(currentPage),
          selectedMenuItem(selectedItem),
          maxItemsPerScreen(maxItemsScreen) {
    }
};
