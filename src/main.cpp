#include <Arduino.h>
#include <SPIFFS.h>
#include <M5Unified.h>


constexpr const char* DK_DISPLAY_IMAGE_PATH = "/image.png";
constexpr const uint32_t DK_DISPLAY_IMAGE_SIZE = 8192;
constexpr const uint8_t DK_SENSOR_INPUT_PIN = 1;
constexpr const uint8_t DK_SENSOR_TIMER_SIZE = 10;


M5Canvas dkDisplayCanvas(&M5.Display);
uint8_t dkDisplayImage[DK_DISPLAY_IMAGE_SIZE];
volatile bool dkSensorIsActive = false;
bool dkSensorWasActive = true;
uint8_t dkSensorRate = 0;
uint8_t dkSensorCounter = 0;
uint32_t dkSensorTimer[DK_SENSOR_TIMER_SIZE] = {};


/** ディスプレイのタスク */
void taskDisplay(void* pvParameters) {
  uint32_t backgroundColor = 0x7ECDF6;
  uint32_t textColor = 0xEE4096;
  int32_t displayW = M5.Display.width();
  int32_t displayH = M5.Display.height();
  while (true) {
    if (dkSensorIsActive != dkSensorWasActive) {
      int32_t imageX = dkSensorIsActive ? 0 : -2;
      int32_t imageY = dkSensorIsActive ? 0 : 2;
      int32_t cursorX = 126 - 12 * String(dkSensorRate).length();
      int32_t cursorY = 4;
      dkDisplayCanvas.createSprite(displayW, displayH);
      dkDisplayCanvas.fillRect(0, 0, displayW, displayH, backgroundColor);
      dkDisplayCanvas.setCursor(cursorX, cursorY);
      dkDisplayCanvas.setTextColor(textColor);
      dkDisplayCanvas.setTextSize(2);
      dkDisplayCanvas.printf("%s\n", dkSensorRate > 0 ? String(dkSensorRate) : "-");
      dkDisplayCanvas.drawPng(dkDisplayImage, DK_DISPLAY_IMAGE_SIZE, imageX, imageY);
      dkDisplayCanvas.pushSprite(0, 0);
      Serial.printf("[INFO] Display updated.\n");
    }
    dkSensorWasActive = dkSensorIsActive;
    dkSensorIsActive = false;
    // Serial.printf("[DEBUG] Display task done.\n");
    delay(100);
  }
}


/** ディスプレイの初期設定 */
void beginDisplay() {
  SPIFFS.begin();
  File file = SPIFFS.open(DK_DISPLAY_IMAGE_PATH);
  file.read(dkDisplayImage, DK_DISPLAY_IMAGE_SIZE);
  file.close();
  M5.Display.setBrightness(64);
  xTaskCreatePinnedToCore(taskDisplay, "taskDisplay", 4096, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
  // Serial.printf("[DEBUG] Display begun.\n");
}


/** センサーのタスク */
void taskSensor() {
  uint32_t previous = dkSensorTimer[dkSensorCounter];
  uint32_t current = millis();
  if (previous > 0) {
    uint8_t rate = (60 * 1000 * DK_SENSOR_TIMER_SIZE) / (current - previous);
    dkSensorRate = rate;
    Serial.printf("<- %d\n", rate);
    Serial.printf("[INFO] Sensor rate calclated. %d\n", rate);
  }
  dkSensorTimer[dkSensorCounter] = current;
  dkSensorCounter = (dkSensorCounter + 1) % DK_SENSOR_TIMER_SIZE;
  dkSensorIsActive = true;
  // Serial.printf("[DEBUG] Sensor task done.\n");
}


/** センサーの初期設定 */
void beginSensor() {
  attachInterrupt(digitalPinToInterrupt(DK_SENSOR_INPUT_PIN), taskSensor, RISING);
  // Serial.printf("[DEBUG] Sensor begun.\n");
}


/** セットアップ */
void setup() {
  M5.begin();
  beginDisplay();
  beginSensor();
  Serial.printf("[INFO] Setup done.\n");
}


/** シリアル通信の監視 */
void observeSerial() {
  if (Serial.available()) {
    String message = Serial.readString();
    if (message.startsWith("ping")) {
      Serial.printf("pong\n");
    }
    Serial.printf("[INFO] Serial message received. %s\n");
  }
  // Serial.printf("[DEBUG] Sensor observed.\n");
}


/** ループ */
void loop() {
  M5.update();
  observeSerial();
}
