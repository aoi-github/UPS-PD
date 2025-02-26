#include <M5Stack.h>
#undef min
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include "Module_4RELAY.h"
#include <M5GFX.h>

// ピンの定義
const int triggerPinL = 5;  // 左側トリガーピン
const int triggerPinR = 13; // 右側トリガーピン

// 状態変数
int sensorValueL = 0;     // 左側センサー値
int sensorValueR = 0;     // 右側センサー値
int delayCount = 0;        // 遅延時間カウント
int bpm = 0;              // BPM値 (Beats Per Minute)、0の場合は通常モード
static int batteryLevel = 0; // バッテリーレベル
unsigned long currentMillis, lastBatteryCheckMillis = 0, lastAirCheckMillisL = 0, lastAirCheckMillisR = 0, lastPacketMillisL = 0, lastPacketMillisR = 0;
unsigned long lastWaveToggleMillis = 0; // 矩形波モードの切り替えタイミング
bool isConnectedL = false; // 左側接続フラグ
bool isConnectedR = false; // 右側接続フラグ
bool wasConnectedL = false; // 左側接続ステータス
bool wasConnectedR = false; // 右側接続ステータス
bool toggleValve = false; 

WebServer server(80); // HTTPサーバー
WiFiUDP udp, udp2;    // UDPオブジェクト

const char ssid[] = "UPSPD5"; // Wi-Fi SSID
const char pass[] = "UPSPDpass"; // Wi-Fi パスワード
const int localPortL = 5000; // 左側ローカルポート
const int localPortR = 5001; // 右側ローカルポート
const IPAddress ip(192, 168, 0, 1); // 静的IPアドレス
const IPAddress subnet(255, 255, 255, 0); // サブネットマスク

extern const char* htmlContent; // HTMLコンテンツの外部定義

void setupWebServer();
void udpListeningTask(void* pvParameters);
void displayBatteryLevel();
void updateDisplay();
String getStatus();
void handleButtonA();
void handleButtonB();
void handleButtonC();
void toggleWave();

void setup() {
  pinMode(triggerPinL, OUTPUT); // 左側トリガーピンの設定
  pinMode(triggerPinR, OUTPUT); // 右側トリガーピンの設定
  Serial.begin(115200);
  M5.begin();
  Wire.begin();
  M5.Lcd.clear();
  digitalWrite(triggerPinL, LOW);
  digitalWrite(triggerPinR, LOW);

  // Wi-Fiアクセスポイントの設定
  WiFi.softAP(ssid, pass);
  delay(50);
  WiFi.softAPConfig(ip, ip, subnet);

  Serial.print("AP IP address: ");
  IPAddress myIP = WiFi.softAPIP();
  Serial.println(myIP);

  // UDPの開始
  Serial.println("Starting UDP");
  udp.begin(localPortL);
  udp2.begin(localPortR);

  // mDNSの設定
  if (!MDNS.begin("m5stack")) {
    Serial.println("Error setting up MDNS responder!");
  } else {
    Serial.println("mDNS responder started");
  }
  MDNS.addService("http", "tcp", 80);

  setupWebServer();

  // HTTPサーバーの開始
  server.begin();

  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextDatum(0);
  xTaskCreatePinnedToCore(udpListeningTask, "udpListeningTask", 4096, NULL, 2, NULL, 1); // コア1で優先度2で実行
  updateDisplay();
}

void loop() {
  currentMillis = millis();

  // 一定間隔でバッテリーレベルを表示
  if (currentMillis - lastBatteryCheckMillis > 5000) {
    displayBatteryLevel();
    lastBatteryCheckMillis = millis();
  }

  // ボタンの状態をチェック
  handleButtonA();
  handleButtonB();
  handleButtonC();
  M5.update();

  // クライアントの処理
  server.handleClient();

  Serial.println("In Normal");

  if (bpm > 0) {
    // 矩形波モード処理
    toggleWave();
  } else {
    // 通常モードの左側センサーの状態をチェック
    if (sensorValueL == 1) {
      if (millis() - lastAirCheckMillisL < 1000) {
        delay(delayCount);
        RELAY.setRelay(0, true);
        Serial.println("sensorOK L");
      } else {
        RELAY.setRelay(0, false);
      }
    } else {
      RELAY.setRelay(0, false);
      lastAirCheckMillisL = millis();
    }

    // 通常モードの右側センサーの状態をチェック
    if (sensorValueR == 1) {
      if (millis() - lastAirCheckMillisR < 1000) {
        delay(delayCount);
        RELAY.setRelay(1, true);
        Serial.println("sensorOK R");
      } else {
        RELAY.setRelay(1, false);
      }
    } else {
      RELAY.setRelay(1, false);
      lastAirCheckMillisR = millis();
    }
  }

  updateDisplay();
}

// Webサーバーのセットアップ
void setupWebServer() {
  server.on("/m5.html", HTTP_GET, []() {
    resetUDP();
    server.send(200, "text/html", htmlContent);
    delay(1); // CPU負荷を軽減
  });

  server.on("/status", HTTP_GET, []() {
    String status = getStatus();
    server.send(200, "application/json", status);
    M5.Lcd.clear();
    updateDisplay();
    delay(1); // CPU負荷を軽減
  });

  server.on("/setDelay", HTTP_GET, []() {
    if (server.hasArg("delay")) {
      delayCount = server.arg("delay").toInt();
      resetUDP();
      Serial.println("Delay set via web to: " + String(delayCount));
      server.send(200, "text/plain", "Delay Set");
    } else {
      server.send(400, "text/plain", "Delay Not Set");
    }
    delay(1); // CPU負荷を軽減
  });

  // BPMの設定
  server.on("/setBPM", HTTP_GET, []() {
    if (server.hasArg("bpm")) {
      bpm = server.arg("bpm").toInt();
      Serial.println("BPM set via web to: " + String(bpm));
      server.send(200, "text/plain", "BPM Set");
    } else {
      server.send(400, "text/plain", "BPM Not Set");
    }
    delay(1); // CPU負荷を軽減
  });

  Serial.println("HTTP server started");
}

// UDPパケットのリスニングタスク
void udpListeningTask(void* pvParameters) {
  while (1) {
    int packetSizeL = udp.parsePacket();
    int packetSizeR = udp2.parsePacket();

    // 左側からパケットを受信した場合の処理
    if (packetSizeL) {
      int portL = udp.remotePort();
      IPAddress remoteIPL = udp.remoteIP();

      Serial.print("Received packet from ");
      Serial.print(remoteIPL);
      Serial.print(":");
      Serial.println(portL);
      if (!isConnectedL) {
        M5.Lcd.clear(); // 接続ステータスが変わった時にリフレッシュ
        isConnectedL = true;
      }
      else if(!wasConnectedL){
        M5.Lcd.clear(); // 接続ステータスが変わった時にリフレッシュ
        wasConnectedL = true;
      }
      lastPacketMillisL = millis();

      while (udp.available()) {
        sensorValueL = udp.read();
        Serial.println("sensorValueL: " + String(sensorValueL));
      }
    }

    // 右側からパケットを受信した場合の処理
    if (packetSizeR) {
      int portR = udp2.remotePort();
      IPAddress remoteIPR = udp2.remoteIP();

      Serial.print("Received packet from ");
      Serial.print(remoteIPR);
      Serial.print(":");
      Serial.println(portR);
      if (!isConnectedR) {
        M5.Lcd.clear(); // 接続ステータスが変わった時にリフレッシュ
        isConnectedR = true;
      }
      else if(!wasConnectedR){
        M5.Lcd.clear(); // 接続ステータスが変わった時にリフレッシュ
        wasConnectedR = true;
      }
      lastPacketMillisR = millis();

      while (udp2.available()) {
        sensorValueR = udp2.read();
        Serial.println("sensorValueR: " + String(sensorValueR));
      }
    }

    delay(1);
  }
}

// バッテリーレベルを表示
void displayBatteryLevel() {
  batteryLevel = M5.Power.getBatteryLevel();
  M5.Lcd.fillRect(200, 0, 200, 30, 0x0000);
  M5.Lcd.drawRightString(String(batteryLevel) + "%", 310, 5, 1);
}

// ディスプレイの更新
void updateDisplay() {
  M5.Lcd.drawString("Delay = " + String(delayCount) + " ms", 0, 120);
  M5.Lcd.drawString(isConnectedL ? "connect L" : "discon L", 0, 200, 1);
  M5.Lcd.drawString(isConnectedR ? "connect R" : "discon R", 170, 200, 1);
}

// 現在のステータスを取得
String getStatus() {
  String status = "{";
  status += "\"battery\": " + String(batteryLevel) + ",";
  status += "\"connectionL\": \"" + String(isConnectedL ? "Connected" : "Disconnected") + "\",";
  status += "\"connectionR\": \"" + String(isConnectedR ? "Connected" : "Disconnected") + "\",";
  status += "\"delay\": " + String(delayCount) + ",";
  status += "\"bpm\": " + String(bpm);
  status += "}";
  return status;
}

// ボタンAの処理
void handleButtonA() {
  if (M5.BtnA.wasPressed()) {
    M5.Lcd.clear();
    resetUDP();
    updateDisplay();
  }
}

// ボタンBの処理
void handleButtonB() {
  if (M5.BtnB.wasPressed()) {
    M5.Lcd.clear();
    resetUDP();
    updateDisplay();
  }
}

// ボタンCの処理
void handleButtonC() {
  if (M5.BtnC.wasPressed()) {
    delayCount = delayCount + 100;
    if (delayCount > 500) {
      delayCount = 0;
    }
    M5.Lcd.clear();
    updateDisplay();
  }
}

// 矩形波モードのトグル処理
void toggleWave() {
  unsigned long interval = (60 * 1000) / bpm; // BPMをミリ秒に変換
  if (currentMillis - lastWaveToggleMillis > interval) {
    if (toggleValve){
    digitalWrite(triggerPinL, HIGH);
    digitalWrite(triggerPinR, LOW);
    toggleValve = !toggleValve;
    }else{
    digitalWrite(triggerPinR, HIGH);
    digitalWrite(triggerPinL, LOW);
    toggleValve = !toggleValve;
    }
    lastWaveToggleMillis = currentMillis;
  }
  
}

// UDPをリセット
void resetUDP() {
  udp.stop();
  udp.begin(localPortL);
  udp2.stop();
  udp2.begin(localPortR);
}
