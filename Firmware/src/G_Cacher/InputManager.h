#pragma once

#include <Arduino.h>
#include "AppState.h"

void updateTouch(AppState &app);
void handleBootButton(AppState &app);
void nextMenu(AppState &app);
void previousMenu(AppState &app);
void goHome(AppState &app);
