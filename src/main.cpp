#include <M5Unified.h>  // M5Unifiedのメインヘッダ
#include <M5GFX.h>      // 画面描画用ライブラリ
#include <WiFi.h>       // Wi-Fi接続
#include <time.h>       // ビルド時刻解析用

#include "wifi_credentials.h"  // Wi-Fi設定（gitignore対象）

M5GFX display;              // ハードウェアディスプレイインスタンス
M5Canvas canvas(&display);  // スプライト描画用キャンバス

constexpr uint32_t WIFI_CONNECT_TIMEOUT_MS = 20000;  // Wi-Fi接続タイムアウト
constexpr uint32_t NTP_SYNC_TIMEOUT_MS = 15000;      // NTP同期タイムアウト
constexpr const char* NTP_PRIMARY = "ntp.nict.jp";   // 優先NTPサーバ
constexpr const char* NTP_SECONDARY = "time.google.com";
constexpr const char* NTP_TERTIARY = "pool.ntp.org";

bool waitForWiFi(uint32_t timeout_ms) {             // Wi-Fi接続完了待ち
  const uint32_t start = millis();                  // 計測開始
  while (WiFi.status() != WL_CONNECTED &&
         (millis() - start) < timeout_ms) {         // タイムアウトまで待機
    delay(250);                                     // 接続完了待ち
  }
  return WiFi.status() == WL_CONNECTED;             // 成否を返す
}

bool syncRtcFromWiFi(uint32_t connect_timeout_ms = WIFI_CONNECT_TIMEOUT_MS,
                     uint32_t ntp_timeout_ms = NTP_SYNC_TIMEOUT_MS) {  // Wi-Fi経由でRTCを同期
  Serial.println("Connecting to Wi-Fi for RTC sync...");               // 状態ログ
  WiFi.mode(WIFI_STA);                                                 // ステーションモード
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);                                // 接続開始
  if (!waitForWiFi(connect_timeout_ms)) {                              // 接続待ち
    Serial.println("Wi-Fi connection timed out.");                     // 失敗ログ
    WiFi.disconnect(true);                                             // クリーンアップ
    WiFi.mode(WIFI_OFF);                                               // 無線OFF
    return false;                                                      // 失敗
  }

  Serial.println("Wi-Fi connected. Syncing time via NTP...");          // NTP同期開始
  configTime(WIFI_GMT_OFFSET_SEC, WIFI_DAYLIGHT_OFFSET_SEC,
             NTP_PRIMARY, NTP_SECONDARY, NTP_TERTIARY);                // タイムゾーン+サーバ設定

  tm timeinfo = {};                                                    // NTP結果格納
  if (!getLocalTime(&timeinfo, ntp_timeout_ms)) {                      // 時刻取得
    Serial.println("Failed to retrieve time from NTP.");               // 取得失敗
    WiFi.disconnect(true);                                             // Wi-Fi停止
    WiFi.mode(WIFI_OFF);                                               // 省電力
    return false;                                                      // 失敗
  }

  m5::rtc_datetime_t rtc(timeinfo);                                  // tm -> RTC形式
  M5.Rtc.setDateTime(&rtc);                                          // RTCへ書き込み
  Serial.println("RTC synced via Wi-Fi.");                           // 成功ログ

  WiFi.disconnect(true);                                             // 以降はRTC使用のため切断
  WiFi.mode(WIFI_OFF);                                               // 省電力
  return true;                                                       // 成功
}

void syncRtcWithBuildTime() {                                   // ビルド時刻をRTCへ書き込む関数
  tm t = {};                                                    // 解析結果を格納するtm構造体
  strptime(__DATE__ " " __TIME__, "%b %d %Y %H:%M:%S", &t);     // __DATE__と__TIME__をtmへパース
  m5::rtc_datetime_t rtc(t);                                    // tmからRTC用構造体に変換
  M5.Rtc.setDateTime(&rtc);                                     // RTCへ反映
}

void setup() {                        // 初期化処理
  auto cfg = M5.config();             // 設定パラメータ取得
  cfg.serial_baudrate = 115200;       // シリアル通信を115200bpsで有効化
  M5.begin(cfg);                      // ボード初期化
  display.begin();                    // ディスプレイ初期化
  display.setRotation(1);             // 画面向きを横向きに設定
  display.setBrightness(128);         // 輝度を中程度に設定

  canvas.setColorDepth(8);                            // スプライト色深度設定
  canvas.createSprite(display.width(), display.height());  // 画面サイズでスプライト確保
  canvas.setTextFont(&fonts::FreeSansBold24pt7b);           // M5GFX同梱フォントを使用

  if (!syncRtcFromWiFi()) {          // Wi-Fi同期を試行して失敗したら
    syncRtcWithBuildTime();          // ビルド時刻でRTCを初期化
  }                                  // Wi-Fi同期に成功した場合は何もしない
}

void loop() {                // メインループ
  M5.update();               // 入力や内部状態を更新

  m5::rtc_time_t rtc;        // RTCから取得する時刻構造体
  if (!M5.Rtc.getTime(&rtc)) {  // RTC読み出しが失敗した場合
    delay(10);               // 10ms待ってから
    return;                  // ループ先頭に戻る
  }

  static int lastSecond = -1;          // 前回描画した秒を記憶
  if (rtc.seconds != lastSecond) {     // 秒が変わったときのみ更新
    lastSecond = rtc.seconds;          // 現在の秒に更新

    canvas.fillScreen(TFT_BLACK);      // 背景を黒でクリア
    char buf[16];                      // 表示用バッファ
    sprintf(buf, "%02d:%02d:%02d", rtc.hours, rtc.minutes, rtc.seconds);  // HH:MM:SS形式に整形
    canvas.setTextDatum(middle_center);  // テキスト基準を中央に設定
    int timeY = canvas.height() * 2 / 5;  // 時計表示位置をやや上に設定
    canvas.drawString(buf, canvas.width() / 2, timeY);  // 文字列を中央の少し上に描画

    int batteryLevel = M5.Power.getBatteryLevel();  // バッテリー残量を取得
    int batteryWidth = canvas.width() * 2 / 4;      // バッテリー枠幅
    int batteryHeight = 50;                         // バッテリー枠高さ
    int batteryX = (canvas.width() - batteryWidth) / 2;  // 枠X座標
    int batteryY = canvas.height() * 3 / 4 - batteryHeight / 2;  // 枠Y座標
    // int terminalWidth = 12;                         // 端子幅
    // int terminalHeight = batteryHeight / 2;         // 端子高さ
    // canvas.drawRoundRect(batteryX, batteryY, batteryWidth, batteryHeight, 8, TFT_WHITE);  // バッテリー枠を描画
    // canvas.fillRect(batteryX + batteryWidth, batteryY + (batteryHeight - terminalHeight) / 2, terminalWidth, terminalHeight, TFT_WHITE);  // 端子を描画
    // int innerPadding = 8;                           // 内側余白
    // int innerWidth = batteryWidth - innerPadding * 2;  // 内側幅
    // int innerHeight = batteryHeight - innerPadding * 2;  // 内側高さ
    // int fillWidth = innerWidth * batteryLevel / 100;  // 充填幅
    // uint16_t fillColor = batteryLevel > 30 ? TFT_GREEN : TFT_RED;  // 30%以下で赤表示
    // if (fillWidth > 0) {                            // 充填幅がある場合のみ塗る
    //   canvas.fillRoundRect(batteryX + innerPadding, batteryY + innerPadding, fillWidth, innerHeight, 6, fillColor);  // 充填領域を描画
    // }                                              // 0なら塗らない
    canvas.drawString(String(batteryLevel) + "%", batteryX + batteryWidth / 2, batteryY + batteryHeight / 2);  // 枠内中央に数値表示

    canvas.pushSprite(0, 0);           // スプライトを画面に転送
  }                                    // 秒が変わらなければ描画しない

  delay(10);                           // 軽負荷のため10msウェイト
}                                      // loop関数終わり
