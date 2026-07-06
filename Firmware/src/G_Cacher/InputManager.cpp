#include "InputManager.h"
#include "Buzzer.h"
#include "Config.h"
#include "Hardware.h"
#include "NetworkManager.h"

static bool readMappedTouch(AppState &app, uint16_t *xOut, uint16_t *yOut)
{
  uint16_t rx, ry;
  if (!tft.getTouch(&rx, &ry)) return false;

  app.rawTouchX = rx;
  app.rawTouchY = ry;

  int x = (int)rx;
  int y = BIG_H - 1 - (int)ry;

  if (TOUCH_INVERT_X)
  {
    x = BIG_W - 1 - x;
  }

  x = constrain(x, 0, BIG_W - 1);
  y = constrain(y, 0, BIG_H - 1);

  *xOut = x;
  *yOut = y;
  return true;
}

void nextMenu(AppState &app)
{
  app.currentMenu = (MenuPage)(((int)app.currentMenu + 1) % MENU_COUNT);
  app.forceRedraw = true;
}

void previousMenu(AppState &app)
{
  int menu = (int)app.currentMenu - 1;
  if (menu < 0) menu = MENU_COUNT - 1;
  app.currentMenu = (MenuPage)menu;
  app.forceRedraw = true;
}

void goHome(AppState &app)
{
  app.currentMenu = MENU_HOME;
  app.forceRedraw = true;
}

static void processTouch(AppState &app, uint16_t x, uint16_t y)
{
  if (y >= NAV_Y)
  {
    clickBeep();
    if (x <= NAV_BACK_X1) previousMenu(app);
    else if (x <= NAV_HOME_X1) goHome(app);
    else nextMenu(app);
    return;
  }

  if (app.currentMenu == MENU_HOME)
  {
    for (int i = 0; i < 5; i++)
    {
      int boxY = 78 + i * 58;
      if (x >= 18 && x <= 302 && y >= boxY && y <= boxY + 42)
      {
        clickBeep();
        app.currentMenu = (MenuPage)(i + 1);
        app.forceRedraw = true;
        return;
      }
    }
  }

  if (app.currentMenu == MENU_SAVE_LOCATIONS)
  {
    if (x >= SAVE_BTN_X && x <= SAVE_BTN_X + SAVE_BTN_W &&
        y >= SAVE_BTN_Y && y <= SAVE_BTN_Y + SAVE_BTN_H)
    {
      requestSaveLocation(app);
      return;
    }
  }
}

void updateTouch(AppState &app)
{
  uint16_t x, y;
  bool pressed = readMappedTouch(app, &x, &y);
  app.touchPressed = pressed;

  if (!pressed)
  {
    app.touchWasDown = false;
    return;
  }

  app.touchOK = true;
  app.touchX = x;
  app.touchY = y;

  if (app.touchWasDown) return;
  app.touchWasDown = true;

  if (millis() - app.lastTouchAction < 220) return;
  app.lastTouchAction = millis();

  processTouch(app, x, y);
}

void handleBootButton(AppState &app)
{
  if (digitalRead(BOOT_BUTTON_PIN) == LOW && millis() - app.lastBootPress > 450)
  {
    app.lastBootPress = millis();
    clickBeep();
    nextMenu(app);
  }
}
