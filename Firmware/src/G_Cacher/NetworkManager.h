#pragma once

#include <Arduino.h>
#include "AppState.h"

void initNetwork(AppState &app);
void updateNetwork(AppState &app);
void requestSaveLocation(AppState &app);
