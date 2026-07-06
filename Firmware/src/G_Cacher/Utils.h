#pragma once

#include <Arduino.h>

void safeCopy(char *dest, size_t destSize, const char *src);
String fixedFloat(bool valid, double value, uint8_t decimals, const char *suffix = nullptr);
String limitText(const char *text, size_t maxLen);
float calculateFSPL(float distanceKm, float frequencyMHz);
