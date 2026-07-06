#pragma once

#include <Arduino.h>
#include <FS.h>
#include "AppState.h"

void initStorage(AppState &app);
void updateStorage(AppState &app);
