#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <PubSubClient.h>
#include <esp_timer.h>
#include <esp_sntp.h>
#include <esp_arduino_version.h>
#include <sys/time.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <atomic>
#include "Config.h"
#if ESP_ARDUINO_VERSION_MAJOR != 3
#error "Use Arduino-ESP32 3.0.7 (3.x timer API)."
#endif
#if !CONFIG_IDF_TARGET_ESP32
#error "This pin map is only for the original dual-core ESP32."
#endif

struct ClockMap { int64_t offsetUs, syncedUs; uint32_t generation; bool valid; };
static ClockMap clockMap{};
static portMUX_TYPE clockMux = portMUX_INITIALIZER_UNLOCKED;
struct Tick { int64_t monoUs, epochUs; uint32_t seq, generation; bool synced; };
struct Sample { Tick tick; float acc[3], gyro[3]; uint16_t force[4]; };
static QueueHandle_t tickQueue, sampleQueue;
static hw_timer_t *sampleTimer;
static WiFiClient tcp;
static PubSubClient mqtt(tcp);
static std::atomic<uint32_t> lateCount{0}, i2cCount{0}, publishFail{0}, published{0};

static void fatal(const char *msg) {
  Serial.printf("STOP: %s\n", msg);
  for (;;) delay(1000);
}
static ClockMap readClock() {
  portENTER_CRITICAL(&clockMux);
  ClockMap c = clockMap;
  portEXIT_CRITICAL(&clockMux);
  return c;
}
static bool clockFresh(const ClockMap &c, int64_t now) {
  return c.valid && now >= c.syncedUs && now - c.syncedUs < SYNC_TTL_US;
}
// Callback runs in task context, never in the hardware timer ISR.
static void onTimeSync(struct timeval *) {
  timeval tv;
  int64_t before = esp_timer_get_time();
  gettimeofday(&tv, nullptr);
  int64_t after = esp_timer_get_time();
  if (tv.tv_sec < 1700000000 || after - before > 1000) return;
  int64_t midpoint = before + (after - before) / 2;
  int64_t offset = int64_t(tv.tv_sec) * 1000000 + tv.tv_usec - midpoint;
  portENTER_CRITICAL(&clockMux);
  clockMap.offsetUs = offset;
  clockMap.syncedUs = midpoint;
  clockMap.generation++;
  clockMap.valid = true;
  portEXIT_CRITICAL(&clockMux);
}
static void ARDUINO_ISR_ATTR onSampleTimer() {
  static uint32_t sequence = 0;
  Tick t;
  t.monoUs = esp_timer_get_time();
  t.seq = sequence++;
  portENTER_CRITICAL_ISR(&clockMux);
  t.epochUs = t.monoUs + clockMap.offsetUs;
  t.generation = clockMap.generation;
  t.synced = clockMap.valid && t.monoUs >= clockMap.syncedUs &&
             t.monoUs - clockMap.syncedUs < SYNC_TTL_US;
  portEXIT_CRITICAL_ISR(&clockMux);
  BaseType_t wake = pdFALSE;
  // One slot: overwrite a missed trigger; never replay old samples in a burst.
  xQueueOverwriteFromISR(tickQueue, &t, &wake);
  if (wake) portYIELD_FROM_ISR();
}
static bool mpuWrite(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(MPU_ADDRESS);
  Wire.write(reg); Wire.write(value);
  return Wire.endTransmission() == 0;
}
static bool mpuRead(uint8_t reg, uint8_t *buf, size_t n) {
  Wire.beginTransmission(MPU_ADDRESS); Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(MPU_ADDRESS, n, true) != n) {
    while (Wire.available()) Wire.read();
    return false;
  }
  for (size_t i = 0; i < n; ++i) buf[i] = Wire.read();
  return true;
}
static void initImu() {
  // Camera SCCB uses controller 1; the external IMU uses Wire/controller 0.
  if (!Wire.begin(IMU_SDA, IMU_SCL, 400000)) fatal("I2C init");
  Wire.setTimeOut(3);
  uint8_t who = 0;
  if (!mpuRead(0x75, &who, 1) || who != 0x68) fatal("MPU6050 WHO_AM_I");
  if (!mpuWrite(0x6B, 0x80)) fatal("MPU reset");
  delay(100);
  // PLL X gyro; awake; DMP/FIFO off; full scale +/-4g and +/-500deg/s.
  // Internal update 1 kHz, read newest register snapshot at 50 Hz.
  // DLPF=1 minimizes group delay; this is NOT a 25Hz anti-alias filter.
  if (!mpuWrite(0x6B, 0x01) || !mpuWrite(0x6C, 0x00) ||
      !mpuWrite(0x6A, 0x00) || !mpuWrite(0x23, 0x00) ||
      !mpuWrite(0x38, 0x00) || !mpuWrite(0x1A, 0x01) ||
      !mpuWrite(0x19, 0x00) || !mpuWrite(0x1B, 0x08) ||
      !mpuWrite(0x1C, 0x08)) fatal("MPU configuration");
  delay(100);
#if !CAP_NODE
  analogReadResolution(12);
  for (uint8_t pin : FORCE_PINS) {
    pinMode(pin, INPUT);
    analogSetPinAttenuation(pin, ADC_11db);
    (void)analogRead(pin); // Initialize ADC before timed acquisition.
  }
#endif
}
static int16_t signed16(const uint8_t *p) {
  return static_cast<int16_t>((uint16_t(p[0]) << 8) | p[1]);
}
static void sensorTask(void *) {
  Tick t;
  for (;;) {
    xQueueReceive(tickQueue, &t, portMAX_DELAY);
    if (!t.synced) continue;
    if (esp_timer_get_time() - t.monoUs > MAX_WAKE_US) { ++lateCount; continue; }
    Sample s{}; s.tick = t;
    uint8_t bytes[14];
    if (!mpuRead(0x3B, bytes, sizeof(bytes))) { ++i2cCount; continue; }
    for (int i = 0; i < 3; ++i) {
      s.acc[i] = signed16(bytes + 2*i) / 8192.0f;
      s.gyro[i] = signed16(bytes + 8 + 2*i) / 65.5f;
    }
#if !CAP_NODE
    for (int i = 0; i < 4; ++i) s.force[i] = analogRead(FORCE_PINS[i]);
#endif
    if (esp_timer_get_time() - t.monoUs > MAX_ACQUIRE_US) { ++lateCount; continue; }
    xQueueOverwrite(sampleQueue, &s);
  }
}
static int formatSample(char *out, size_t cap, const Sample &s) {
#if CAP_NODE
  return snprintf(out, cap,
    "{\"t_ms\":%lld,\"seq\":%lu,\"acc\":[%.4f,%.4f,%.4f],\"gyro\":[%.3f,%.3f,%.3f]}",
    (long long)(s.tick.epochUs / 1000), (unsigned long)s.tick.seq,
    s.acc[0], s.acc[1], s.acc[2], s.gyro[0], s.gyro[1], s.gyro[2]);
#else
  return snprintf(out, cap,
    "{\"t_ms\":%lld,\"seq\":%lu,\"acc\":[%.4f,%.4f,%.4f],\"gyro\":[%.3f,%.3f,%.3f],\"force\":[%u,%u,%u,%u]}",
    (long long)(s.tick.epochUs / 1000), (unsigned long)s.tick.seq,
    s.acc[0], s.acc[1], s.acc[2], s.gyro[0], s.gyro[1], s.gyro[2],
    unsigned(s.force[0]), unsigned(s.force[1]), unsigned(s.force[2]), unsigned(s.force[3]));
#endif
}
static void networkTask(void *) {
  uint32_t retryAt = 0, wifiRetryAt = 0, reportAt = 0;
  int64_t lastEpoch = 0;
  for (;;) {
    uint32_t now = millis();
    if (WiFi.status() != WL_CONNECTED) {
      tcp.stop();
      if (int32_t(now - wifiRetryAt) >= 0) { WiFi.reconnect(); wifiRetryAt = now + 5000; }
    } else if (!mqtt.connected()) {
      if (int32_t(now - retryAt) >= 0) {
        // PubSubClient publishes only QoS 0. Connect/publish never block the sensor task.
        if (MQTT_USER[0]) mqtt.connect(CLIENT_ID, MQTT_USER, MQTT_PASSWORD);
        else mqtt.connect(CLIENT_ID);
        retryAt = millis() + 2000;
      }
    } else mqtt.loop();
    Sample s;
    if (xQueueReceive(sampleQueue, &s, pdMS_TO_TICKS(2)) == pdTRUE) {
      ClockMap c = readClock();
      int64_t current = esp_timer_get_time();
      if (mqtt.connected() && clockFresh(c, current) &&
          c.generation == s.tick.generation &&
          current - s.tick.monoUs < MAX_SEND_AGE_US && s.tick.epochUs > lastEpoch) {
        char payload[256];
        int n = formatSample(payload, sizeof(payload), s);
        if (n > 0 && size_t(n) < sizeof(payload) && mqtt.publish(DATA_TOPIC, payload, false)) {
          lastEpoch = s.tick.epochUs; ++published;
        } else ++publishFail;
      }
      // Disconnected/stale samples are discarded, without disk or backlog.
    }
    if (int32_t(now - reportAt) >= 0) {
      ClockMap c = readClock();
      Serial.printf("pub=%lu late=%lu i2c=%lu pubFail=%lu sync=%d heap=%u minHeap=%u\n",
        (unsigned long)published.load(), (unsigned long)lateCount.load(),
        (unsigned long)i2cCount.load(), (unsigned long)publishFail.load(),
        clockFresh(c, esp_timer_get_time()), ESP.getFreeHeap(), ESP.getMinFreeHeap());
      reportAt = now + 5000;
    }
    vTaskDelay(1);
  }
}
static void initNetworkAndClock() {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  IPAddress ip(192,168,4,NODE_IP_LAST), gateway(192,168,4,1), mask(255,255,255,0);
  if (!WiFi.config(ip, gateway, mask, gateway)) fatal("Static IP");
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Waiting for Wi-Fi...");
  uint32_t retry = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    if (millis() - retry > 15000) { WiFi.reconnect(); retry = millis(); }
  }
  sntp_set_time_sync_notification_cb(onTimeSync);
  sntp_set_sync_interval(60000);
  configTime(0, 0, GATEWAY_HOST); // Offline NTP; UDP 123. No Internet fallback.
  Serial.println("Waiting for a valid NTP response from gateway...");
  while (!clockFresh(readClock(), esp_timer_get_time())) delay(100);
  tcp.setConnectionTimeout(1000);
  tcp.setTimeout(1000);
  mqtt.setServer(GATEWAY_HOST, 1883);
  mqtt.setSocketTimeout(1);
  mqtt.setKeepAlive(15);
  if (!mqtt.setBufferSize(384)) fatal("MQTT allocation");
}
static void startSensing() {
  tickQueue = xQueueCreate(1, sizeof(Tick));
  sampleQueue = xQueueCreate(1, sizeof(Sample));
  if (!tickQueue || !sampleQueue) fatal("Queue allocation");
  if (xTaskCreatePinnedToCore(sensorTask, "sensor", 4096, nullptr, 4, nullptr, 1) != pdPASS ||
      xTaskCreatePinnedToCore(networkTask, "mqtt", 4096, nullptr, 1, nullptr, 0) != pdPASS)
    fatal("Task allocation");
  sampleTimer = timerBegin(1000000);
  if (!sampleTimer) fatal("Timer allocation");
  timerAttachInterrupt(sampleTimer, &onSampleTimer);
  timerAlarm(sampleTimer, PERIOD_US, true, 0);
}
