#pragma once

#include <Arduino.h>
#include "AppState.h"

void drawCurrentScreen(AppState &app);
void updateUi(AppState &app);
uint16_t colourFromServer(const char *name);
