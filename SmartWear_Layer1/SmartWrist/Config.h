#pragma once
// Confirm the actual BOM hardware before enabling acquisition.
constexpr bool HARDWARE_CONFIRMED = false;
// Wrist: four conditioned FSR analog outputs, powered at 3.3 V.
// Cap: AI Thinker-compatible ESP32-CAM + OV2640 + working PSRAM.
constexpr char WIFI_SSID[] = "SmartWear-LAN";
constexpr char WIFI_PASSWORD[] = "CHANGE_ME";
constexpr char GATEWAY_HOST[] = "192.168.4.1";
constexpr char MQTT_USER[] = "";
constexpr char MQTT_PASSWORD[] = "";
constexpr uint32_t PERIOD_US = 20000;
constexpr int64_t SYNC_TTL_US = 120000000;
constexpr int64_t MAX_WAKE_US = 2000;
constexpr int64_t MAX_ACQUIRE_US = 5000;
constexpr int64_t MAX_SEND_AGE_US = 40000;
constexpr uint8_t MPU_ADDRESS = 0x68;
#if CAP_NODE
constexpr uint8_t NODE_IP_LAST = 21;
constexpr int IMU_SDA = 13, IMU_SCL = 14;
constexpr char DATA_TOPIC[] = "wearable/user01/cap/imu";
constexpr char CLIENT_ID[] = "smartwear-user01-cap";
#else
constexpr uint8_t NODE_IP_LAST = 22;
constexpr int IMU_SDA = 21, IMU_SCL = 22;
constexpr uint8_t FORCE_PINS[4] = {32, 33, 34, 35};
constexpr char DATA_TOPIC[] = "wearable/user01/wrist/data";
constexpr char CLIENT_ID[] = "smartwear-user01-wrist";
#endif
