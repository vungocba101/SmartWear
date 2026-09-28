#define CAP_NODE 1
#include "Sensing.h"
#include "CameraStream.h"

void setup() {
  Serial.begin(115200);
  delay(300);
  if (!HARDWARE_CONFIRMED) fatal("Verify AI Thinker-compatible board, OV2640 and PSRAM; read README then enable config");
  initImu();
  initCamera();
  initNetworkAndClock();
  startSensing();
  startCameraServer();
}
void loop() { delay(1000); }
