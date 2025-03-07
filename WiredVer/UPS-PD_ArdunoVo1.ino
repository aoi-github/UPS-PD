const int sensorPinL = A1;  // 圧力センサーのアナログ入力ピン
const int sensorPinR = A0;  // 圧力センサーのアナログ入力ピン
const int valveL = 10;       // バルブのデジタル出力ピン
const int valveR = 9;       // バルブのデジタル出力ピン
int sensorValueL = 0;       // センサーの値を格納する変数
int sensorValueR = 0;       // センサーの値を格納する変数
unsigned long lastcloseTimeL = 0;  // 左バルブが最後に開いた時間
unsigned long lastcloseTimeR = 0;  // 右バルブが最後に開いた時間
const unsigned long opentime = 1000; // 2000msの開閉制限

void setup() {
    pinMode(valveL, OUTPUT);
    pinMode(valveR, OUTPUT);
    Serial.begin(9600);
}

void loop() {
    sensorValueL = analogRead(sensorPinL);  // 左センサーの値を読み取る
    sensorValueR = analogRead(sensorPinR);  // 右センサーの値を読み取る
    Serial.println("L : " + String(sensorValueL) + " R : " + String(sensorValueR)); // シリアルモニタに出力


    // 左バルブの制御
    if (sensorValueL <= 500) {
      if(millis() - lastcloseTimeL <= opentime){
        digitalWrite(valveL, 0);
      }
      else{
        digitalWrite(valveL, 1);
      }
  
    } 
    else {
        digitalWrite(valveL, 1);
        lastcloseTimeL = millis();  // 開いた時間を記録
    }

    // 右バルブの制御
    if (sensorValueR <= 500) {
      if(millis() - lastcloseTimeR <= opentime){
        digitalWrite(valveR, 0);
      }
      else{
        digitalWrite(valveR, 1);
      }
  
    } 
    else {
        digitalWrite(valveR, 1);
        lastcloseTimeR = millis();  // 開いた時間を記録
    }

    delay(100);
}
