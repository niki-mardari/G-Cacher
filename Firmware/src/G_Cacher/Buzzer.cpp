#include "Buzzer.h"
#include "Config.h"

struct MelodyNote
{
  uint16_t frequency;
  uint16_t durationMs;
};

const MelodyNote TARGET_MELODY[] = {
  {880, 90},
  {1175, 90},
  {1568, 140},
  {0, 70},
  {1568, 180}
};

constexpr uint8_t TARGET_MELODY_COUNT = sizeof(TARGET_MELODY) / sizeof(TARGET_MELODY[0]);

bool melodyActive = false;
uint8_t melodyIndex = 0;
unsigned long nextMelodyStepMs = 0;

void initBuzzer()
{
  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);
}

void clickBeep()
{
  tone(BUZZER_PIN, BUZZER_CLICK_FREQ, BUZZER_CLICK_MS);
}

void errorBeep()
{
  tone(BUZZER_PIN, 420, 80);
}

void playStartupMelody()
{
  const MelodyNote startup[] = {
    {988, 70},
    {1175, 70},
    {1319, 70},
    {1568, 120}
  };

  for (uint8_t i = 0; i < sizeof(startup) / sizeof(startup[0]); i++)
  {
    tone(BUZZER_PIN, startup[i].frequency, startup[i].durationMs);
    delay(startup[i].durationMs + 25);
  }
  noTone(BUZZER_PIN);
}

void playTargetReachedMelody()
{
  melodyActive = true;
  melodyIndex = 0;
  nextMelodyStepMs = 0;
}

void updateBuzzer()
{
  if (!melodyActive) return;
  if (millis() < nextMelodyStepMs) return;

  if (melodyIndex >= TARGET_MELODY_COUNT)
  {
    noTone(BUZZER_PIN);
    melodyActive = false;
    return;
  }

  const MelodyNote &note = TARGET_MELODY[melodyIndex++];
  if (note.frequency == 0)
  {
    noTone(BUZZER_PIN);
  }
  else
  {
    tone(BUZZER_PIN, note.frequency, note.durationMs);
  }

  nextMelodyStepMs = millis() + note.durationMs + 35;
}
