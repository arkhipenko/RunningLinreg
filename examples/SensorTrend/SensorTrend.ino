//
// SensorTrend: how fast is a sensor reading changing?
//
// Reads A0 every 100 ms and fits a line to the last 50 readings (5 seconds).
// The slope is the rate of change in ADC counts per second.
// The window is allocated from the heap by the constructor.
//
// t is seconds since start-up rather than raw millis(): on AVR a double
// is 32-bit and holds about 7 significant digits, so a small t keeps
// its resolution for longer.
//

#include <RunningLinreg.h>

const int SENSOR_PIN = A0;
const unsigned long PERIOD_MS = 100;

RunningLinreg trend(50);
unsigned long start;

void setup() {
  Serial.begin(115200);
  if ( trend.status() == LINREG_NUL ) {
    Serial.println(F("Not enough memory for the window"));
  }
  start = millis();
}

void loop() {
  double t = (millis() - start) / 1000.0;
  trend.addPoint({ t, (double) analogRead(SENSOR_PIN) });

  if ( trend.status() == LINREG_OK ) {
    Serial.print(F("rate="));
    Serial.print(trend.slope(), 3);
    Serial.print(F(" counts/s"));
  }
  else {
    Serial.print(F("status="));
    Serial.print(trend.status());
  }
  Serial.print(F(" points="));
  Serial.print((unsigned long) trend.count());
  Serial.print('/');
  Serial.println((unsigned long) trend.window());

  delay(PERIOD_MS);
}
