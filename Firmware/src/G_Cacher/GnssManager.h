#pragma once

#include <Arduino.h>
#include "AppState.h"

void initGnss(AppState &app);
void updateGnss(AppState &app);
bool gnssReceiving(const AppState &app);
bool gnssHasFix(const AppState &app);
int gnssSatellites();
float gnssHdop();
String gnssTimeText();
String gnssDateText();
const char *qualityText(const AppState &app);
const char *qualityReason(const AppState &app);
uint16_t qualityColour(const AppState &app);
