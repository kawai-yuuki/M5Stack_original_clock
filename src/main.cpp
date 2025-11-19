#include <M5Unified.h>
#include <M5GFX.h>
#include <WiFi.h>
#include <esp_sntp.h>
#include <time.h>

#include "wifi_credentials.h"

namespace {

constexpr const char* kNtpServer = "ntp.nict.jp";
constexpr uint32_t kWifiConnectTimeoutMs = 15000;
constexpr uint32_t kNtpTimeoutMs = 10000;
constexpr uint32_t kResyncIntervalMs = 60 * 60 * 1000;  // 1 hour

uint32_t gLastSyncMillis = 0;
time_t gLastSyncedEpoch = 0;
bool gRtcValid = false;
M5Canvas gCanvas(&M5.Display);
bool gCanvasReady = false;

void logTime(const char* label, const tm& info) {
  char buffer[32];
  strftime(buffer, sizeof(buffer), "%Y/%m/%d %H:%M:%S", &info);
  Serial.printf("[%s] %s\n", label, buffer);
}

void showStatus(const char* message, uint16_t textSize = 2) {
  auto& lcd = M5.Display;
  lcd.fillScreen(BLACK);
  lcd.setTextColor(WHITE, BLACK);
  lcd.setTextSize(textSize);
  lcd.setCursor(10, lcd.height() / 2 - 10);
  lcd.print(message);
  Serial.printf("[STATUS] %s\n", message);
}

bool initCanvas() {
  gCanvas.setColorDepth(16);
  if (gCanvas.createSprite(M5.Display.width(), M5.Display.height()) == nullptr) {
    Serial.println("[Canvas] Failed to allocate sprite");
    return false;
  }
  gCanvasReady = true;
  return true;
}

bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  const uint32_t start = millis();
  Serial.printf("[WiFi] Connecting to %s\n", WIFI_SSID);
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - start > kWifiConnectTimeoutMs) {
      Serial.println("[WiFi] Connection timeout");
      return false;
    }
    delay(250);
  }
  Serial.printf("[WiFi] Connected. IP: %s\n", WiFi.localIP().toString().c_str());
  return true;
}

bool syncTime(tm* out) {
  Serial.println("[NTP] Sync start");
  configTime(WIFI_GMT_OFFSET_SEC, WIFI_DAYLIGHT_OFFSET_SEC, kNtpServer);
  sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);
  sntp_restart();
  const uint32_t start = millis();
  while (millis() - start <= kNtpTimeoutMs) {
    if (getLocalTime(out)) {
      const int year = out->tm_year + 1900;
      if (year >= 2020 && year < 2100 &&
          sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
        logTime("NTP", *out);
        return true;
      }
      Serial.printf("[NTP] Ignore invalid year %d\n", year);
    }
    delay(500);
  }
  Serial.println("[NTP] Sync timeout");
  return false;
}

bool readTimeFromRtc(tm* out) {
  if (!out || !M5.Rtc.isEnabled()) {
    return false;
  }
  m5::rtc_datetime_t datetime;
  if (!M5.Rtc.getDateTime(&datetime)) {
    return false;
  }
  *out = datetime.get_tm();
  logTime("RTC", *out);
  return true;
}

void updateRtcFrom(const tm& info) {
  if (!M5.Rtc.isEnabled()) {
    return;
  }
  M5.Rtc.setDateTime(&info);
  gRtcValid = true;
  logTime("RTC set", info);
}

void updateCache(const tm& info) {
  tm copy = info;
  time_t epoch = mktime(&copy);
  if (epoch <= 0) {
    return;
  }
  gLastSyncedEpoch = epoch;
  gLastSyncMillis = millis();
}

bool getCachedTime(tm* out) {
  if (!out || gLastSyncedEpoch <= 0) {
    return false;
  }
  const uint32_t elapsed = millis() - gLastSyncMillis;
  time_t current = gLastSyncedEpoch + elapsed / 1000;
  return localtime_r(&current, out);
}

template <typename Target>
void drawBatteryLevel(Target& target) {
  const int level = M5.Power.getBatteryLevel();
  const bool charging = M5.Power.isCharging();
  char battBuf[24];
  if (level < 0) {
    snprintf(battBuf, sizeof(battBuf), "BAT N/A");
  } else {
    snprintf(battBuf, sizeof(battBuf), "%s %d%%", charging ? "CHG" : "BAT", level);
  }
  target.setTextColor(GREEN, BLACK);
  target.setTextDatum(top_right);
  target.setTextSize(2);
  target.drawString(battBuf, target.width() - 10, 20);
}

template <typename Target>
void renderTimeTo(Target& target, const tm& info) {
  target.fillScreen(BLACK);
  target.setTextColor(WHITE, BLACK);
  target.setTextDatum(middle_center);

  char dateBuf[32];
  strftime(dateBuf, sizeof(dateBuf), "%Y/%m/%d (%a)", &info);
  target.setTextSize(3);
  target.drawString(dateBuf, target.width() / 2, target.height() / 3);

  char timeBuf[16];
  strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", &info);
  target.setTextSize(5);
  target.drawString(timeBuf, target.width() / 2, target.height() * 2 / 3);

  drawBatteryLevel(target);
}

void drawTime(const tm& info) {
  if (gCanvasReady) {
    renderTimeTo(gCanvas, info);
    gCanvas.pushSprite(0, 0);
  } else {
    renderTimeTo(M5.Display, info);
  }
}

}  // namespace

void setup() {
  auto cfg = M5.config();
  cfg.serial_baudrate = 115200;
  M5.begin(cfg);
  Serial.println("[BOOT] Device start");
  M5.Display.setRotation(1);
  M5.Display.setBrightness(160);
  initCanvas();

  tm rtcTime;
  if (readTimeFromRtc(&rtcTime)) {
    drawTime(rtcTime);
    updateCache(rtcTime);
    gRtcValid = true;
  } else {
    showStatus("RTC not set");
  }

  showStatus("Connecting Wi-Fi...");
  if (!connectWiFi()) {
    showStatus("Wi-Fi failed");
    return;
  }

  showStatus("Syncing NTP...");
  tm timeinfo;
  if (!syncTime(&timeinfo)) {
    showStatus("NTP failed");
    return;
  }
  updateRtcFrom(timeinfo);
  updateCache(timeinfo);

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);

  showStatus("Time synced");
  delay(500);
  drawTime(timeinfo);
}

void loop() {
  static uint32_t lastDraw = 0;
  const uint32_t now = millis();
  const bool wifiConnected = (WiFi.status() == WL_CONNECTED);
  if (now - lastDraw >= 1000) {
    lastDraw = now;
    tm timeinfo;
    if (wifiConnected && getLocalTime(&timeinfo)) {
      updateRtcFrom(timeinfo);
      updateCache(timeinfo);
      drawTime(timeinfo);
    } else if (getCachedTime(&timeinfo)) {
      drawTime(timeinfo);
    } else if (gRtcValid && readTimeFromRtc(&timeinfo)) {
      drawTime(timeinfo);
    }
  }

  if (wifiConnected && now - gLastSyncMillis >= kResyncIntervalMs) {
    tm synced;
    if (syncTime(&synced)) {
      updateRtcFrom(synced);
      updateCache(synced);
    }
  }
}
