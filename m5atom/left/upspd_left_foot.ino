#include <M5Atom.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#define ANALOG_PIN 32

int count = 0;
int val = 0;
const char ssid[] = "UPSPD1"; // SSID
const char pass[] = "UPSPDpass";  // password

static WiFiUDP wifiUdp;
static const char *kRemoteIpadr = "192.168.0.1";  //送信先のIPアドレス 
static const int kRmoteUdpPort = 1000; //送信先のポート　機器によって変更する
int portbox[] = {1,0,0,0}; //上のポートと一緒になっているかを確認，数字は1桁ずつ入れる

extern const unsigned numdigits[10][25];

void numdigitsdraw(int number, int color){
  for(int x = 0; x < 5; x++){

    for(int y = 0; y < 5; y++){

      if(numdigits[number][x + y * 5]==1){
        M5.dis.drawpix(x,y, color);
      }else{
        M5.dis.drawpix(x,y, 0x000000);
      }
    }
  }
}

void numlight(){ //ポートの番号ライトアップする
 
  int portboxLength = sizeof(portbox) / sizeof(portbox[0]);
  Serial.println(portboxLength);

  for(int count = 0; count < portboxLength; count++){
    numdigitsdraw(portbox[count], 0xf00000);
    wifiUdp.beginPacket(kRemoteIpadr, kRmoteUdpPort);
    wifiUdp.write(0);
    delay(300); // 800より小さくしないとWiFiが切れる→WiFiを800ms受信しないとdisconになるようにしている
    wifiUdp.endPacket();
    wifiUdp.beginPacket(kRemoteIpadr, kRmoteUdpPort);
    wifiUdp.write(0);
    M5.dis.clear();
    delay(200);
    wifiUdp.endPacket();


  }
  delay(700); // 800より小さくしないとWiFiが切れる→WiFiを800ms受信しないとdisconになるようにしている

}

static void WiFi_setup(){ //Wi-Fiのセットアップ
  static const int kLocalPort = 5000;  //自身のポート
  WiFi.begin(ssid, pass);
  int attempts = 0;
  while ( WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    attempts++;
    if (attempts > 20) {  // 20 * 500ms = 10秒でタイムアウト
      Serial.println("WiFi connection failed. Restarting...");
      ESP.restart();
    }
  }
  Serial.println("WiFi connected");
  wifiUdp.begin(kLocalPort);
}

static void Serial_setup(){
  Serial.begin(115200);
}

// void wifiloop(void * pvParameters){
//   while(1){
//     wifiUdp.beginPacket(kRemoteIpadr, kRmoteUdpPort);
//     wifiUdp.endPacket();
//     delay(1000);  // 1秒に1回送信
//   }
// }

void setup() {
  M5.begin(true, false, true);
  Wire.begin();
  Serial_setup();
  WiFi_setup();
  delay(100);
}

void loop() {
  M5.update();

  val = analogRead(ANALOG_PIN);
  Serial.println(val);
  if (val < 75 ) {
    wifiUdp.beginPacket(kRemoteIpadr, kRmoteUdpPort);
    wifiUdp.write(1);
    wifiUdp.endPacket();
    delay(10);
  }
  else {
    wifiUdp.beginPacket(kRemoteIpadr, kRmoteUdpPort);
    wifiUdp.write(0);
    wifiUdp.endPacket();
    delay(10);
  }

  if(M5.Btn.wasPressed()){ //2秒ボタンを押してから離すとポート番号が表示
    wifiUdp.beginPacket(kRemoteIpadr, kRmoteUdpPort);
    numlight();
    Serial.println("Pressd");
    wifiUdp.endPacket();
  }

  delay(50);
}

