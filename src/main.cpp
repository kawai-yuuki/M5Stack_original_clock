// #include <M5Unified.h>  // M5Unifiedのメインヘッダ
// #include <M5GFX.h>      // 画面描画用ライブラリ
// #include <WiFi.h>       // Wi-Fi接続
// #include <time.h>       // NTP時刻取得用

// #include "wifi_credentials.h"  // Wi-Fi設定（gitignore対象）

// M5GFX display;              // ハードウェアディスプレイインスタンス
// M5Canvas canvas(&display);  // スプライト描画用キャンバス

// constexpr const char* NTP_PRIMARY = "ntp.nict.jp";   // 優先NTPサーバ
// constexpr const char* NTP_SECONDARY = "time.google.com";
// constexpr const char* NTP_TERTIARY = "pool.ntp.org";

// void drawStatusMessage(const char* message) {
//   canvas.fillScreen(TFT_BLACK);
//   canvas.setTextDatum(top_left);
//   canvas.drawString(message, 10, 10);
//   canvas.pushSprite(0, 0);
// }

// bool waitForWiFi(uint32_t timeout_ms = 20000) {
//   const uint32_t start = millis();
//   while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeout_ms) {
//     delay(250);
//   }
//   return WiFi.status() == WL_CONNECTED;
// }

// bool getAdjustedLocalTime(struct tm* result) {
//   if (result == nullptr) {
//     return false;
//   }
//   time_t now = time(nullptr);
//   if (now <= 0) {
//     return false;
//   }
//   now += WIFI_GMT_OFFSET_SEC + WIFI_DAYLIGHT_OFFSET_SEC;
//   gmtime_r(&now, result);
//   return true;
// }

// // void configureTimezone() {
// //   if (WIFI_TIMEZONE[0] != '\0') {                           // POSIX形式のTZがある場合
// //     Serial.print("Using timezone string: ");
// //     Serial.println(WIFI_TIMEZONE);
// //     setenv("TZ", WIFI_TIMEZONE, 1);                         // TZ環境変数設定
// //     tzset();                                                // libcへ即反映
// //   } else {
// //     Serial.println("Using manual UTC offset only (TZ string empty).");
// //   }
// //   configTime(0, 0, NTP_PRIMARY, NTP_SECONDARY, NTP_TERTIARY);  // SNTPは常にUTCで取得
// // }

// // bool waitForWiFi(uint32_t timeout_ms) {             // Wi-Fi接続完了待ち
// //   const uint32_t start = millis();                  // 計測開始
// //   while (WiFi.status() != WL_CONNECTED &&
// //          (millis() - start) < timeout_ms) {         // タイムアウトまで待機
// //     delay(250);                                     // 接続完了待ち
// //   }
// //   return WiFi.status() == WL_CONNECTED;             // 成否を返す
// // }

// // bool syncRtcFromWiFi(uint32_t connect_timeout_ms = WIFI_CONNECT_TIMEOUT_MS,
// //                      uint32_t ntp_timeout_ms = NTP_SYNC_TIMEOUT_MS) {  // Wi-Fi経由でRTCを同期
// //   Serial.println("Connecting to Wi-Fi for RTC sync...");               // 状態ログ
// //   WiFi.mode(WIFI_STA);                                                 // ステーションモード
// //   WiFi.begin(WIFI_SSID, WIFI_PASSWORD);                                // 接続開始
// //   if (!waitForWiFi(connect_timeout_ms)) {                              // 接続待ち
// //     Serial.println("Wi-Fi connection timed out.");                     // 失敗ログ
// //     WiFi.disconnect(true);                                             // クリーンアップ
// //     WiFi.mode(WIFI_OFF);                                               // 無線OFF
// //     return false;                                                      // 失敗
// //   }

// //   Serial.print("Wi-Fi connected. IP: ");
// //   Serial.println(WiFi.localIP());                                      // 接続情報
// //   Serial.println("Syncing time via NTP...");                           // NTP同期開始
// //   // configureTimezone();

// //   tm timeinfo = {};                                                    // 同期待機用
// //   if (!getLocalTime(&timeinfo, ntp_timeout_ms)) {                      // 時刻取得
// //     Serial.println("Failed to retrieve time from NTP.");               // 取得失敗
// //     WiFi.disconnect(true);                                             // Wi-Fi停止
// //     WiFi.mode(WIFI_OFF);                                               // 省電力
// //     return false;                                                      // 失敗
// //   }

// //   // SNTP完了後のEpochからUTC/ローカルを両方計算
// //   time_t now = time(nullptr);
// //   tm utcTime = {};
// //   gmtime_r(&now, &utcTime);
// //   char utcBuf[32];
// //   strftime(utcBuf, sizeof(utcBuf), "%Y-%m-%d %H:%M:%S", &utcTime);
// //   Serial.print("UTC time: ");
// //   Serial.println(utcBuf);

// //   // タイムゾーン適用 (TZ文字列→ダメなら手動オフセット)
// //   tm localTime = {};
// //   bool tzResult = false;
// //   if (WIFI_TIMEZONE[0] != '\0') {
// //     tzResult = localtime_r(&now, &localTime) != nullptr;
// //   }
// //   bool shouldFallbackToManual =
// //       !tzResult ||
// //       ((WIFI_GMT_OFFSET_SEC != 0 || WIFI_DAYLIGHT_OFFSET_SEC != 0) &&
// //        localTime.tm_year == utcTime.tm_year && localTime.tm_yday == utcTime.tm_yday &&
// //        localTime.tm_hour == utcTime.tm_hour && localTime.tm_min == utcTime.tm_min);
// //   if (shouldFallbackToManual) {
// //     time_t shifted = now + WIFI_GMT_OFFSET_SEC + WIFI_DAYLIGHT_OFFSET_SEC;
// //     gmtime_r(&shifted, &localTime);
// //     Serial.println("Local time (manual offset) will be used.");
// //   } else {
// //     Serial.println("Local time (TZ string) will be used.");
// //   }
// //   char localBuf[32];
// //   strftime(localBuf, sizeof(localBuf), "%Y-%m-%d %H:%M:%S", &localTime);
// //   Serial.print("Local time: ");
// //   Serial.println(localBuf);

// //   m5::rtc_datetime_t rtc(localTime);                                  // tm -> RTC形式
// //   M5.Rtc.setDateTime(&rtc);                                          // RTCへ書き込み
// //   Serial.println("RTC synced via Wi-Fi.");                           // 成功ログ

// //   WiFi.disconnect(true);                                             // 以降はRTC使用のため切断
// //   WiFi.mode(WIFI_OFF);                                               // 省電力
// //   return true;                                                       // 成功
// // }

// // void syncRtcWithBuildTime() {                                   // ビルド時刻をRTCへ書き込む関数
// //   tm t = {};                                                    // 解析結果を格納するtm構造体
// //   strptime(__DATE__ " " __TIME__, "%b %d %Y %H:%M:%S", &t);     // __DATE__と__TIME__をtmへパース
// //   m5::rtc_datetime_t rtc(t);                                    // tmからRTC用構造体に変換
// //   M5.Rtc.setDateTime(&rtc);                                     // RTCへ反映
// // }

// void setup() {                        // 初期化処理
//   auto cfg = M5.config();             // 設定パラメータ取得
//   cfg.serial_baudrate = 115200;       // シリアル通信を115200bpsで有効化
//   M5.begin(cfg);                      // ボード初期化
//   display.begin();                    // ディスプレイ初期化
//   display.setRotation(1);             // 画面向きを横向きに設定
//   display.setBrightness(128);         // 輝度を中程度に設定

//   canvas.setColorDepth(8);                            // スプライト色深度設定
//   canvas.createSprite(display.width(), display.height());  // 画面サイズでスプライト確保
//   canvas.setFont(&fonts::FreeSansBold24pt7b);               // M5GFX同梱フォントを使用
//   canvas.setTextColor(TFT_WHITE, TFT_BLACK);

//   drawStatusMessage("Connecting to Wi-Fi...");
//   WiFi.mode(WIFI_STA);
//   WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
//   if (!waitForWiFi()) {
//     drawStatusMessage("Wi-Fi timeout");
//   } else {
//     drawStatusMessage("Wi-Fi connected");
//   }
//   delay(500);

//   configTime(0, 0, NTP_PRIMARY, NTP_SECONDARY, NTP_TERTIARY);  // UTCベースで同期
//   drawStatusMessage("Syncing time...");
//   struct tm dummy = {};
//   if (!getLocalTime(&dummy, 10000)) {
//     drawStatusMessage("Failed to get time");
//   } else {
//     drawStatusMessage("Time synced");
//   }
//   delay(500);

//   WiFi.disconnect(true);                                             // 以降はRTC使用のため切断
//   WiFi.mode(WIFI_OFF);                                               // 省電力
// }

// void loop() {                                  // メインループ
//   static uint32_t lastDraw = 0;                // 前回描画した時刻
//   uint32_t nowMs = millis();                   // 現在の経過時間
//   if (nowMs - lastDraw < 1000) {               // 1秒周期でのみ描画
//     delay(10);
//     return;
//   }
//   lastDraw = nowMs;

//   struct tm timeinfo = {};                     // ローカル時刻
//   if (!getAdjustedLocalTime(&timeinfo)) {      // 取得失敗時はメッセージを表示
//     canvas.fillScreen(TFT_BLACK);
//     canvas.setTextDatum(middle_center);
//     canvas.drawString("Time read error", canvas.width() / 2, canvas.height() / 2);
//     canvas.pushSprite(0, 0);
//     return;
//   }

//   canvas.fillScreen(TFT_BLACK);                // 背景クリア
//   canvas.setTextDatum(middle_center);          // 中央基準で描画
//   char buf[16];
//   snprintf(buf, sizeof(buf), "%02d:%02d:%02d",
//            timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
//   int timeY = canvas.height() * 2 / 5;
//   canvas.drawString(buf, canvas.width() / 2, timeY);  // 時計を表示

//   int batteryLevel = M5.Power.getBatteryLevel();      // バッテリー残量
//   int batteryWidth = canvas.width() * 2 / 4;
//   int batteryHeight = 50;
//   int batteryX = (canvas.width() - batteryWidth) / 2;
//   int batteryY = canvas.height() * 3 / 4 - batteryHeight / 2;
//   canvas.drawString(String(batteryLevel) + "%",        // シンプルに％のみ表示
//                     batteryX + batteryWidth / 2,
//                     batteryY + batteryHeight / 2);

//   canvas.pushSprite(0, 0);                             // 画面へ転送
// }



#include <M5Unified.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <time.h>

#include "wifi_credentials.h"

namespace {

constexpr const char* kNtpServer = "ntp.jst.mfeed.ad.jp";
constexpr uint32_t kWifiTimeoutMs = 20000;
constexpr uint32_t kNtpTimeoutMs = 10000;
const char* kWeekdayNames[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

WiFiMulti wifiMulti;
m5::rtc_date_t rtcDate;
m5::rtc_time_t rtcTime;

void showMessage(const char* message, uint16_t textSize = 2) {
  auto& lcd = M5.Display;
  lcd.fillScreen(BLACK);
  lcd.setTextColor(WHITE, BLACK);
  lcd.setTextSize(textSize);
  lcd.setCursor(10, 40);
  lcd.print(message);
}

bool waitForWiFi() {
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < kWifiTimeoutMs) {
    wifiMulti.run();
    delay(250);
  }
  return WiFi.status() == WL_CONNECTED;
}

bool syncRtcFromNtp() {
  configTime(WIFI_GMT_OFFSET_SEC, WIFI_DAYLIGHT_OFFSET_SEC, kNtpServer);
  tm timeinfo = {};
  uint32_t start = millis();
  while (!getLocalTime(&timeinfo)) {
    if (millis() - start > kNtpTimeoutMs) {
      return false;
    }
    delay(500);
  }

  rtcDate.year = timeinfo.tm_year + 1900;
  rtcDate.month = timeinfo.tm_mon + 1;
  rtcDate.date = timeinfo.tm_mday;
  rtcDate.weekDay = timeinfo.tm_wday;
  M5.Rtc.setDate(&rtcDate);

  rtcTime.hours = timeinfo.tm_hour;
  rtcTime.minutes = timeinfo.tm_min;
  rtcTime.seconds = timeinfo.tm_sec;
  M5.Rtc.setTime(&rtcTime);
  return true;
}

void drawClock() {
  auto& lcd = M5.Display;
  lcd.fillScreen(BLACK);
  lcd.setTextColor(WHITE, BLACK);

  if (!M5.Rtc.getDate(&rtcDate) || !M5.Rtc.getTime(&rtcTime)) {
    showMessage("RTC read error");
    return;
  }

  char dateBuf[32];
  snprintf(dateBuf, sizeof(dateBuf), "%04d.%02d.%02d %s",
           rtcDate.year, rtcDate.month, rtcDate.date,
           kWeekdayNames[rtcDate.weekDay % 7]);
  lcd.setTextSize(3);
  lcd.setTextDatum(middle_center);
  lcd.drawString(dateBuf, lcd.width() / 2, lcd.height() / 3);

  char timeBuf[16];
  snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d",
           rtcTime.hours, rtcTime.minutes, rtcTime.seconds);
  lcd.setTextSize(5);
  lcd.drawString(timeBuf, lcd.width() / 2, lcd.height() * 2 / 3);
}

}  // namespace

void setup() {
  auto cfg = M5.config();
  cfg.serial_baudrate = 115200;
  M5.begin(cfg);
  M5.Display.setRotation(1);
  M5.Display.setBrightness(160);

  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);
  WiFi.mode(WIFI_STA);
  showMessage("Connecting Wi-Fi...");

  if (waitForWiFi()) {
    showMessage("Wi-Fi connected");
    delay(300);
    if (!syncRtcFromNtp()) {
      showMessage("NTP failed");
      delay(1000);
    } else {
      showMessage("RTC updated");
      delay(500);
    }
  } else {
    showMessage("Wi-Fi timeout");
    delay(1000);
  }

  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

void loop() {
  static uint32_t lastDraw = 0;
  if (millis() - lastDraw >= 500) {
    lastDraw = millis();
    drawClock();
  }
  delay(10);
}
