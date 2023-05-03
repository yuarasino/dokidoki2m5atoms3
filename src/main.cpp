#include <Arduino.h>
#include <SPIFFS.h>
#include <M5Unified.h>


// 定数

const auto D_DISPLAY_IMAGE_PATH = "/image.png";
const auto D_DISPLAY_IMAGE_SIZE = 8192;
const auto D_SENSOR_INPUT_PIN = 1;
const auto D_SENSOR_TIMER_SIZE = 5;


// グローバル変数

volatile bool gDisplayIsPulsing = true;
volatile bool gSensorIsPulsing = false;
volatile int gSensorRate = 0;

M5Canvas gDisplayCanvas(&M5.Display);
uint8_t gDisplayImage[D_DISPLAY_IMAGE_SIZE] = {};
int gSensorCounter = 0;
unsigned long gSensorTimer[D_SENSOR_TIMER_SIZE] = {};


/** シリアルの送信 */
void sendSerial(int rate) {
  auto message = String("<- ") + String(rate);
  Serial.println(message);
}


/** ディスプレイの更新 */
void updateDisplay(void* pvParameters) {
  while (true) {
    if (gSensorIsPulsing != gDisplayIsPulsing) {
      auto imageX = gSensorIsPulsing ? 0 : -2;
      auto imageY = gSensorIsPulsing ? 0 : 2;
      auto rateText = gSensorRate ? String(gSensorRate) : String("-");
      gDisplayCanvas.createSprite(128, 128);
      gDisplayCanvas.fillScreen(uint32_t(0x7ECDF6));
      gDisplayCanvas.setTextColor(uint32_t(0xEE4096));
      gDisplayCanvas.setTextSize(2);
      gDisplayCanvas.drawRightString(rateText, 126, 4);
      gDisplayCanvas.drawPng(gDisplayImage, D_DISPLAY_IMAGE_SIZE, imageX, imageY);
      gDisplayCanvas.pushSprite(0, 0);
      // Serial.printf("[DEBUG] Display canvas reflected.\n");
    }
    gDisplayIsPulsing = gSensorIsPulsing;
    gSensorIsPulsing = false;
    delay(100);
  }
}


/** センサーの更新 */
void updateSensor() {
  auto currentTime = millis();
  auto prevTime = gSensorTimer[gSensorCounter];
  if (prevTime > 0) {
    auto rate = int((60 * 1000 * D_SENSOR_TIMER_SIZE) / (currentTime - prevTime));
    gSensorRate = rate;
    // Serial.print("[DEBUG] Sensor rate measured. ");
    // Serial.println(rate);
    sendSerial(rate);
  }
  gSensorTimer[gSensorCounter++] = currentTime;
  gSensorCounter %= D_SENSOR_TIMER_SIZE;
  gSensorIsPulsing = true;
}


/** ディスプレイの設定 */
void beginDisplay() {
  auto file = SPIFFS.open(D_DISPLAY_IMAGE_PATH);
  file.read(gDisplayImage, D_DISPLAY_IMAGE_SIZE);
  file.close();
  M5.Display.setBrightness(64);
  xTaskCreatePinnedToCore(updateDisplay, "updateDisplay", 4096, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
  Serial.println("[INFO] Display begun.");
}


/** センサーの設定 */
void beginSensor() {
  attachInterrupt(digitalPinToInterrupt(D_SENSOR_INPUT_PIN), updateSensor, RISING);
  Serial.println("[INFO] Sensor begun.");
}


/** セットアップ */
void setup() {
  SPIFFS.begin();
  M5.begin();
  beginDisplay();
  beginSensor();
}


/** シリアルの更新 */
void updateSerial() {
  if (Serial.available()) {
    auto message = Serial.readString();
    Serial.println(message);
  }
}


/** ループ */
void loop() {
  M5.update();
  updateSerial();
  delay(100);
}
