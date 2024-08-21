#include "menuTypes.h"
#include "globals.h"
#include "lcd_utils.h"
#include "wifi_utils.h"

#define NULL ((void*)0)

void submenu1Fn() {
    menuSystem.currentMenu = &submenu1;
    displayMenu(&menuSystem, 0);
}

void submenu2Fn() {
    menuSystem.currentMenu = &submenu2;
    displayMenu(&menuSystem, 0);
}

void submenu3Fn() {
    menuSystem.currentMenu = &submenu3;
    displayMenu(&menuSystem, 0);
}

void volver() {
    if (menuSystem.currentMenu == &mainMenu) {
        // If in the main menu, call the printCurrentIP function
        printCurrentIP();
    } else if (menuSystem.currentMenu->parent) {
        // Navigate to the parent menu
        menuSystem.currentMenu = menuSystem.currentMenu->parent;
        displayMenu(&menuSystem, 0);
    }
}

void initMenuSystem(MenuSystem *system) {
    system->currentMenu = &mainMenu;
}

void selectMenuItem(MenuSystem *system, int index) {
    if (index >= 0 && index < system->currentMenu->itemCount) {
        system->currentMenu->items[index].action();
    } else {
        printf("Invalid selection.\n");
    }
}

void displayMenu(MenuSystem *system, int selectedIndex) {
    // Clear the LCD before displaying the menu
    lcd_clear();

    // Display the menu name on the first line of the LCD
    lcd_put_cur(0, 0); // Move cursor to the beginning of the first line
    lcd_send_string(system->currentMenu->name);

    // Display the selected menu item on the second line of the LCD
    for (int i = 0; i < system->currentMenu->itemCount; i++) {
        if (i == selectedIndex) {
            lcd_put_cur(1, 0); // Move cursor to the beginning of the second line
            lcd_send_string(system->currentMenu->items[i].name); // Highlight the selected item
        }
    }
}
