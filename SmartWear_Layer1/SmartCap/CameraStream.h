#pragma once
#include <esp_camera.h>
#include <esp_http_server.h>
#if defined(CONFIG_SCCB_HARDWARE_I2C_PORT0) && CONFIG_SCCB_HARDWARE_I2C_PORT0
#error "Camera SCCB configured on I2C0, conflicting with Wire IMU. Use the pinned core with SCCB on I2C1."
#endif

static void initCamera() {
  if (!psramFound()) fatal("PSRAM required for two JPEG frame buffers");
  camera_config_t c{};
  c.ledc_channel = LEDC_CHANNEL_0; c.ledc_timer = LEDC_TIMER_0;
  // ONLY AI Thinker ESP32-CAM pin map, pending physical board confirmation.
  c.pin_d0 = 5; c.pin_d1 = 18; c.pin_d2 = 19; c.pin_d3 = 21;
  c.pin_d4 = 36; c.pin_d5 = 39; c.pin_d6 = 34; c.pin_d7 = 35;
  c.pin_xclk = 0; c.pin_pclk = 22; c.pin_vsync = 25; c.pin_href = 23;
  c.pin_sccb_sda = 26; c.pin_sccb_scl = 27;
  c.pin_pwdn = 32; c.pin_reset = -1;
  c.xclk_freq_hz = 20000000;
  c.pixel_format = PIXFORMAT_JPEG;
  c.frame_size = FRAMESIZE_QVGA;
  c.jpeg_quality = 15; // Larger number: lower quality, less network traffic.
  c.fb_count = 2; c.fb_location = CAMERA_FB_IN_PSRAM;
  c.grab_mode = CAMERA_GRAB_LATEST;
  esp_err_t err = esp_camera_init(&c);
  if (err != ESP_OK) { Serial.printf("camera err=0x%x\n", err); fatal("Camera init"); }
  sensor_t *sensor = esp_camera_sensor_get();
  if (!sensor || sensor->id.PID != OV2640_PID) fatal("Expected OV2640");
  // AEC/AGC stay automatic. Actual FPS depends on illumination/exposure and Wi-Fi.
}
static esp_err_t streamHandler(httpd_req_t *req) {
  ClockMap initial = readClock();
  if (!clockFresh(initial, esp_timer_get_time())) {
    httpd_resp_set_status(req, "503 Service Unavailable");
    return httpd_resp_send(req, "NTP not fresh", HTTPD_RESP_USE_STRLEN);
  }
  httpd_resp_set_type(req, "multipart/x-mixed-replace;boundary=smartwear");
  httpd_resp_set_hdr(req, "Cache-Control", "no-store");
  uint32_t frameSeq = 0, frames = 0;
  int64_t windowStart = esp_timer_get_time(), lastFrameEpoch = 0;
  for (;;) {
    if (WiFi.status() != WL_CONNECTED) return ESP_FAIL;
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) return ESP_FAIL;
    ClockMap map = readClock();
    int64_t now = esp_timer_get_time();
    // esp32-camera timestamp: monotonic time when first DMA buffer was received,
    // NOT Unix time, not an exact exposure midpoint. See README validation gate.
    int64_t captureUs = int64_t(fb->timestamp.tv_sec) * 1000000 + fb->timestamp.tv_usec;
    int64_t epochUs = captureUs + map.offsetUs;
    if (!clockFresh(map, now) || captureUs <= 0 || captureUs > now ||
        now - captureUs > 200000 || epochUs <= lastFrameEpoch) {
      esp_camera_fb_return(fb);
      return ESP_FAIL; // Force client reconnect after sync loss or clock reversal.
    }
    char header[256];
    int n = snprintf(header, sizeof(header),
      "\r\n--smartwear\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n"
      "X-Timestamp-Ms: %lld\r\nX-Frame-Seq: %lu\r\n\r\n",
      unsigned(fb->len), (long long)(epochUs / 1000), (unsigned long)frameSeq++);
    esp_err_t result = ESP_FAIL;
    if (n > 0 && size_t(n) < sizeof(header)) {
      result = httpd_resp_send_chunk(req, header, n);
      if (result == ESP_OK)
        result = httpd_resp_send_chunk(req, reinterpret_cast<const char *>(fb->buf), fb->len);
    }
    esp_camera_fb_return(fb); // Returned on every acquired-frame path; no JPEG copy.
    if (result != ESP_OK) return result;
    lastFrameEpoch = epochUs;
    ++frames;
    now = esp_timer_get_time();
    if (now - windowStart >= 5000000) {
      Serial.printf("MJPEG sent fps=%.2f psramFree=%u\n",
        frames * 1000000.0 / (now - windowStart), ESP.getFreePsram());
      frames = 0; windowStart = now;
    }
    vTaskDelay(1);
  }
}
static void startCameraServer() {
  httpd_config_t cfg = HTTPD_DEFAULT_CONFIG();
  cfg.server_port = 81;
  cfg.ctrl_port = 32769;
  cfg.core_id = 0;
  cfg.task_priority = 2;
  cfg.stack_size = 6144;
  cfg.max_open_sockets = 1; // One gateway consumer; no browser viewer simultaneously.
  cfg.max_uri_handlers = 1;
  cfg.send_wait_timeout = 2;
  cfg.recv_wait_timeout = 2;
  httpd_handle_t server = nullptr;
  if (httpd_start(&server, &cfg) != ESP_OK) fatal("HTTP server");
  httpd_uri_t uri{};
  uri.uri = "/stream"; uri.method = HTTP_GET; uri.handler = streamHandler;
  if (httpd_register_uri_handler(server, &uri) != ESP_OK) fatal("Stream handler");
  Serial.println("MJPEG: http://192.168.4.21:81/stream");
}
