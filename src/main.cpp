#include <M5Unified.h>  // M5Unifiedのメインヘッダ
#include <M5GFX.h>      // 画面描画用ライブラリ
#include <time.h>       // ビルド時刻解析用

M5GFX display;              // ハードウェアディスプレイインスタンス
M5Canvas canvas(&display);  // スプライト描画用キャンバス

bool syncRtcFromSerial(uint32_t timeout_ms = 15000) {                                     // シリアル経由でPC時刻を取得して同期
  const char* format = "YYYY-MM-DD HH:MM:SS";                                             // 受信フォーマット説明
  Serial.printf("Send timestamp in %s format within %u ms\r\n", format, timeout_ms);      // PC側へ指示メッセージ
  uint32_t start = millis();                                                              // タイムアウト計測開始
  char buf[32] = {};                                                                      // 受信用バッファ
  size_t idx = 0;                                                                         // バッファ書き込み位置
  while ((millis() - start) < timeout_ms && idx < sizeof(buf) - 1) {                      // タイムアウトまで待機
    if (Serial.available()) {                                                             // 受信データがあれば処理
      char c = Serial.read();                                                             // 1文字読み出し
      if (c == '\n' || c == '\r') {                                                       // 改行で入力終了
        break;                                                                            // ループを抜ける
      }                                                                                   // 改行で終了
      buf[idx++] = c;                                                                     // バッファへ格納
    } else {                                                                              // データが無いとき
      delay(5);                                                                           // CPU負荷軽減の小休止
    }                                                                                     // 無通信時処理終了
  }                                                                                       // 入力待ちループ終わり
  buf[idx] = '\0';                                                                        // 文字列終端
  int year, month, day, hour, minute, second;                                             // 分解後の値
  if (sscanf(buf, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second) == 6) {  // 形式が合っていれば
    m5::rtc_date_t date(year, month, day);                                                // 日付構造体へ格納
    m5::rtc_time_t time(hour, minute, second);                                            // 時刻構造体へ格納
    M5.Rtc.setDateTime(&date, &time);                                                     // RTCへ書き込み
    Serial.println("RTC synced from serial input.");                                      // 成功メッセージ
    return true;                                                                          // 成功を返す
  }                                                                                       // 解析成功時処理終了
  Serial.println("RTC sync failed or timed out.");                                        // 失敗を通知
  return false;                                                                           // 失敗としてfalseを返す
}                                                                                         // 関数終わり

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

  if (!syncRtcFromSerial()) {        // シリアル同期を試行して失敗したら
    syncRtcWithBuildTime();          // ビルド時刻でRTCを初期化
  }                                  // シリアル同期に成功した場合は何もしない
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
    int terminalWidth = 12;                         // 端子幅
    int terminalHeight = batteryHeight / 2;         // 端子高さ
    canvas.drawRoundRect(batteryX, batteryY, batteryWidth, batteryHeight, 8, TFT_WHITE);  // バッテリー枠を描画
    canvas.fillRect(batteryX + batteryWidth, batteryY + (batteryHeight - terminalHeight) / 2, terminalWidth, terminalHeight, TFT_WHITE);  // 端子を描画
    int innerPadding = 8;                           // 内側余白
    int innerWidth = batteryWidth - innerPadding * 2;  // 内側幅
    int innerHeight = batteryHeight - innerPadding * 2;  // 内側高さ
    int fillWidth = innerWidth * batteryLevel / 100;  // 充填幅
    uint16_t fillColor = batteryLevel > 30 ? TFT_GREEN : TFT_RED;  // 30%以下で赤表示
    if (fillWidth > 0) {                            // 充填幅がある場合のみ塗る
      canvas.fillRoundRect(batteryX + innerPadding, batteryY + innerPadding, fillWidth, innerHeight, 6, fillColor);  // 充填領域を描画
    }                                              // 0なら塗らない
    canvas.drawString(String(batteryLevel) + "%", batteryX + batteryWidth / 2, batteryY + batteryHeight / 2);  // 枠内中央に数値表示

    canvas.pushSprite(0, 0);           // スプライトを画面に転送
  }                                    // 秒が変わらなければ描画しない

  delay(10);                           // 軽負荷のため10msウェイト
}                                      // loop関数終わり
