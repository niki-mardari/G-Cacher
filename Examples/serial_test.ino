#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

// TENSTAR / ESP32-S3 onboard NeoPixel data pin
const int NEOPIXEL_PIN = 33;

// If  LED does not turn on, keep this enabled.
const int NEOPIXEL_POWER_PIN = 34;

// There is 1 onboard NeoPixel
const int NUM_PIXELS = 1;

Adafruit_NeoPixel pixel(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

unsigned long startTime;
unsigned long endTime;
unsigned long totalTime;
float individualTime;

int x = 0;

// Simple function to change colour based on x
void setNeoPixelColor(int value) {
  int colourNumber = value % 6;

  if (colourNumber == 0) {
    pixel.setPixelColor(0, pixel.Color(20, 0, 0));     // Red
  }
  else if (colourNumber == 1) {
    pixel.setPixelColor(0, pixel.Color(0, 20, 0));     // Green
  }
  else if (colourNumber == 2) {
    pixel.setPixelColor(0, pixel.Color(0, 0, 20));     // Blue
  }
  else if (colourNumber == 3) {
    pixel.setPixelColor(0, pixel.Color(20, 20, 0));    // Yellow
  }
  else if (colourNumber == 4) {
    pixel.setPixelColor(0, pixel.Color(0, 20, 20));    // Cyan
  }
  else {
    pixel.setPixelColor(0, pixel.Color(20, 0, 20));    // Purple
  }

  pixel.show();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Enable NeoPixel power if your board uses GPIO34 for NeoPixel power
  pinMode(NEOPIXEL_POWER_PIN, OUTPUT);
  digitalWrite(NEOPIXEL_POWER_PIN, HIGH);

  // Start NeoPixel
  pixel.begin();
  pixel.clear();
  pixel.show();

  Serial.println("TENSTAR ESP32-S3 NeoPixel test started");
}

void loop() {
  startTime = micros();

  for (int i = 0; i < 1000; i++) {
    x++;
  }

  endTime = micros();

  totalTime = endTime - startTime;
  individualTime = (float)totalTime / 1000.0;

  // Change NeoPixel colour after each batch of increments
  setNeoPixelColor(x);

  Serial.print("Total time for 1000 adds: ");
  Serial.print(totalTime);
  Serial.println(" us");

  Serial.print("Estimated time per single x++: ");
  Serial.print(individualTime, 4);
  Serial.println(" us");

  Serial.print("Current x value: ");
  Serial.println(x);

  Serial.println("-----------------------");

  delay(500);
}