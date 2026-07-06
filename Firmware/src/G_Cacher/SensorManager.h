#pragma once

#include <Arduino.h>
#include "AppState.h"

void initSensors(AppState &app);
void updateSensors(AppState &app);
const char *thermalStatus(const AppState &app);
uint16_t thermalColour(const AppState &app);
