#include <Arduino.h>
#include <SPIFFS.h>
#include <M5Unified.h>


const char* D_DISPLAY_IMAGE_PATH = "/image.png";
const int D_DISPLAY_IMAGE_SIZE = 8192;
const int D_SENSOR_INPUT_PIN = 1;
const int D_SENSOR_TIMER_SIZE = 4;


volatile bool gDisplayIsPulsed = true;
M5Canvas gDisplayCanvas(&M5.Display);
uint8_t gDisplayImage[D_DISPLAY_IMAGE_SIZE] = {};
volatile bool gSensorIsMeasured = true;
int gSensorRate = 0;
int gSensorCounter = 0;
unsigned long gSensorTimer[D_SENSOR_TIMER_SIZE] = {};


/** ディスプレイのタスク */
void taskDisplay(void* pvParameters) {
  while (true) {
    if (gDisplayIsPulsed) {
      auto imageX = gSensorIsMeasured ? 0 : -2;
      auto imageY = gSensorIsMeasured ? 0 : 2;
      auto sensorText = gSensorRate ? String(gSensorRate) : String("-");
      auto sensorX = 126 - 12 * sensorText.length();
      gDisplayCanvas.createSprite(128, 128);
      gDisplayCanvas.fillRect(0, 0, 128, 128, 0x7ECDF6UL);
      gDisplayCanvas.setCursor(sensorX, 4);
      gDisplayCanvas.setTextColor(0xEE4096UL);
      gDisplayCanvas.setTextSize(2);
      gDisplayCanvas.printf("%s\n", sensorText);
      gDisplayCanvas.drawPng(gDisplayImage, D_DISPLAY_IMAGE_SIZE, imageX, imageY);
      gDisplayCanvas.pushSprite(0, 0);
      Serial.printf("[INFO] Display updated.\n");
    }
    if (!gSensorIsMeasured) {
      gDisplayIsPulsed = false;
    }
    gSensorIsMeasured = false;
    // Serial.printf("[DEBUG] Display task done.\n");
    delay(100);
  }
}


/** センサーのタスク */
void taskSensor() {
  auto previousTime = gSensorTimer[gSensorCounter];
  auto currentTime = millis();
  if (previousTime > 0) {
    auto rate = (int)((60 * 1000 * D_SENSOR_TIMER_SIZE) / (currentTime - previousTime));
    gSensorRate = rate;
    Serial.printf("<- %d\n", rate);
    Serial.printf("[INFO] Sensor rate measured. %d\n", rate);
  }
  gSensorTimer[gSensorCounter++] = currentTime;
  gSensorCounter %= D_SENSOR_TIMER_SIZE;
  gSensorIsMeasured = true;
  gDisplayIsPulsed =  true;
  // Serial.printf("[DEBUG] Sensor task done.\n");
}


/** ディスプレイの初期設定 */
void beginDisplay() {
  SPIFFS.begin();
  auto file = SPIFFS.open(D_DISPLAY_IMAGE_PATH);
  file.read(gDisplayImage, D_DISPLAY_IMAGE_SIZE);
  file.close();
  M5.Display.setBrightness(64);
  xTaskCreatePinnedToCore(taskDisplay, "taskDisplay", 4096, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
  // Serial.printf("[DEBUG] Display begun.\n");
}


/** センサーの初期設定 */
void beginSensor() {
  attachInterrupt(digitalPinToInterrupt(D_SENSOR_INPUT_PIN), taskSensor, RISING);
  // Serial.printf("[DEBUG] Sensor begun.\n");
}


/** セットアップ */
void setup() {
  M5.begin();
  beginDisplay();
  beginSensor();
  // Serial.printf("[DEBUG] M5 begun.\n");
}


/** シリアルの状態更新 */
void updateSerial() {
  if (Serial.available()) {
    auto message = Serial.readString();
    Serial.printf("[INFO] Serial message received. %s\n");
  }
  // Serial.printf("[DEBUG] Serial updated.\n");
}


/** ループ */
void loop() {
  M5.update();
  updateSerial();
  // Serial.printf("[DEBUG] M5 updated.\n");
  delay(100);
}
