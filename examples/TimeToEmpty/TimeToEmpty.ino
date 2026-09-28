//
// TimeToEmpty: predict when a battery reaches its cut-off voltage.
//
// Samples the battery voltage on A0 every 10 seconds and fits a line to
// the last 30 samples (5 minutes). Where the line crosses the cut-off
// voltage is the predicted time to empty.
// The window lives in a static array owned by the sketch, so no heap is used.
//

#include <RunningLinreg.h>

const int     BATTERY_PIN = A0;
const double  VOLTS_PER_COUNT = 5.0 / 1023.0 * 2.0;  // 5 V reference, 1:2 divider
const double  CUTOFF_VOLTS = 3.3;
const unsigned long PERIOD_MS = 10000UL;
const size_t  WINDOW = 30;

point_t storage[WINDOW];
RunningLinreg battery(WINDOW, storage);
unsigned long start;

void setup() {
  Serial.begin(115200);
  start = millis();
}

void loop() {
  double t = (millis() - start) / 1000.0;
  double volts = analogRead(BATTERY_PIN) * VOLTS_PER_COUNT;
  battery.addPoint({ t, volts });

  Serial.print(F("volts="));
  Serial.print(volts, 3);

  //  value = slope * t + intercept reaches the cut-off at t = (cut-off - intercept) / slope
  if ( battery.status() != LINREG_OK ) {
    Serial.println(F(" collecting samples"));
  }
  else if ( battery.slope() >= 0 ) {
    Serial.println(F(" not discharging"));
  }
  else {
    double tEmpty = (CUTOFF_VOLTS - battery.intercept()) / battery.slope();
    Serial.print(F(" empty in "));
    Serial.print((tEmpty - t) / 60.0, 1);
    Serial.println(F(" min"));
  }

  delay(PERIOD_MS);
}
