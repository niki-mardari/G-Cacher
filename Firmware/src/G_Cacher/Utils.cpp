#include "Utils.h"
#include <math.h>
#include <string.h>

void safeCopy(char *dest, size_t destSize, const char *src)
{
  if (destSize == 0) return;
  if (src == nullptr)
  {
    dest[0] = '\0';
    return;
  }

  strncpy(dest, src, destSize - 1);
  dest[destSize - 1] = '\0';
}

String fixedFloat(bool valid, double value, uint8_t decimals, const char *suffix)
{
  if (!valid) return "--";

  String out = String(value, static_cast<unsigned int>(decimals));
  if (suffix != nullptr) out += suffix;
  return out;
}

String limitText(const char *text, size_t maxLen)
{
  if (text == nullptr) return "";

  String out(text);
  if (out.length() > maxLen)
  {
    out = out.substring(0, maxLen - 3);
    out += "...";
  }
  return out;
}

float calculateFSPL(float distanceKm, float frequencyMHz)
{
  return 32.44f + 20.0f * log10(distanceKm) + 20.0f * log10(frequencyMHz);
}
