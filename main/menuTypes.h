#ifndef MENUTYPES_H
#define MENUTYPES_H

#ifdef __cplusplus
extern "C"
{
#endif

    // Function pointer type for menu item actions
    typedef void (*MenuFunction)();

    // MenuItem structure definition
    typedef struct MenuItem
    {
        const char *name;         // Name of the menu item
        MenuFunction action;      // Function to call when this item is selected
        struct SubMenu *submenu;  // Pointer to a submenu, if this item leads to one
    } MenuItem;

    // SubMenu structure definition
    typedef struct SubMenu
    {
        const char *name;       // Name of the submenu
        MenuItem *items;        // Array of menu items in this submenu
        int itemCount;          // Number of items in this submenu
        struct SubMenu *parent; // Pointer to the parent menu (for "VOLVER")
    } SubMenu;

    // MenuSystem structure definition
    typedef struct MenuSystem
    {
        SubMenu *currentMenu; // Pointer to the current submenu
    } MenuSystem;

    // Function declarations
    void volver();

    void selectMenuItem(MenuSystem *menuSystem, int index); // Function to select a menu item

    void initMenuSystem(MenuSystem *menuSystem); // Function to initialize the menu system

    void displayMenu(MenuSystem *menuSystem, int selectedIndex); // Function to display the menu

    // Example actions
    void submenu1Fn();
    void submenu2Fn();
    void submenu3Fn();

#ifdef __cplusplus
}
#endif
#endif
