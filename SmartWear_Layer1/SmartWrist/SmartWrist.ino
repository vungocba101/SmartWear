#define CAP_NODE 0
#include "Sensing.h"

void setup() {
  Serial.begin(115200);
  delay(300);
  if (!HARDWARE_CONFIRMED) fatal("Read README, verify FSR analog circuitry and board, then set HARDWARE_CONFIRMED=true");
  initImu();
  initNetworkAndClock();
  startSensing();
}
void loop() { delay(1000); }
