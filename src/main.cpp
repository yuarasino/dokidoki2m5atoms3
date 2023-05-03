#include <Arduino.h>
#include <SPIFFS.h>
#include <M5Unified.h>
#include <BLEDevice.h>
#include <BLE2902.h>


// 定数

const auto D_BLE_DEVICE_NAME = "dokidoki";
const auto D_BLE_SERVICE_UUID = "da43aef5-6f7c-5e04-a350-1284e36c1f05";
const auto D_BLE_CHARACTERISTIC_UUID = "d21408ba-c9bb-e952-4762-3168a403b42e";
const auto D_LCD_IMAGE_PATH = "/image.png";
const auto D_LCD_IMAGE_SIZE = 8192;
const auto D_HRS_INPUT_PIN = 1;
const auto D_HRS_TIMER_SIZE = 4;


// グローバル変数

volatile bool gBleIsAdvertising = false;
volatile bool gBleIsConnecting = false;
volatile bool gLcdWasPulsing = true;
volatile bool gHrsIsPulsing = false;
volatile int gHrsRate = 0;

M5Canvas gLcdCanvas(&M5.Lcd);
uint8_t gLcdImage[D_LCD_IMAGE_SIZE] = {};
BLECharacteristic* gBleCharacteristic;
int gHrsCounter = 0;
unsigned long gHrsTimer[D_HRS_TIMER_SIZE] = {};


/** ディスプレイのタスク */
void taskLcd(void* pvParameters) {
  while (true) {
    if (gHrsIsPulsing != gLcdWasPulsing) {
      auto imageX = gHrsIsPulsing ? 0 : -2;
      auto imageY = gHrsIsPulsing ? 0 : 2;
      auto connText = gBleIsConnecting ? String("~") : String("-");
      auto rateText = gHrsRate ? String(gHrsRate) : String("-");
      gLcdCanvas.createSprite(128, 128);
      gLcdCanvas.fillScreen(uint32_t(0x7ECDF6));
      gLcdCanvas.setTextColor(uint32_t(0xEE4096));
      gLcdCanvas.setTextSize(2);
      gLcdCanvas.drawString(connText, 4, 4);
      gLcdCanvas.drawRightString(rateText, 126, 4);
      gLcdCanvas.drawPng(gLcdImage, D_LCD_IMAGE_SIZE, imageX, imageY);
      gLcdCanvas.pushSprite(0, 0);
      Serial.printf("Lcd canvas pushed.\n");
    }
    gLcdWasPulsing = gHrsIsPulsing;
    gHrsIsPulsing = false;
    delay(100);
  }
}


/** Bluetoothの接続コールバック */
class CBleServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* server) {
    gBleIsConnecting = true;
    Serial.printf("BLE connected.\n");
  }
  void onDisconnect(BLEServer* server) {
    gBleIsConnecting = false;
    gBleIsAdvertising = false;
    Serial.printf("BLE disconnected.\n");
  }
};


/** Bluetoothの通知を送信 */
void notifyBle(int rate) {
  if (gBleIsConnecting) {
    auto message = String("->") + String(rate);
    gBleCharacteristic->setValue(message.c_str());
    gBleCharacteristic->notify();
    Serial.printf("BLE message notified. %s\n", message);
  }
}


/** 心拍数センサーの割り込み */
void interruptHrs() {
  auto currentTime = millis();
  auto prevTime = gHrsTimer[gHrsCounter];
  if (prevTime > 0) {
    auto rate = int((60 * 1000 * D_HRS_TIMER_SIZE) / (currentTime - prevTime));
    notifyBle(rate);
    gHrsRate = rate;
    Serial.printf("HRS rate calculated. %d\n", rate);
  }
  gHrsTimer[gHrsCounter++] = currentTime;
  gHrsCounter %= D_HRS_TIMER_SIZE;
  gHrsIsPulsing = true;
}


/** ディスプレイの初期設定 */
void beginLcd() {
  auto file = SPIFFS.open(D_LCD_IMAGE_PATH);
  file.read(gLcdImage, D_LCD_IMAGE_SIZE);
  file.close();
  M5.Lcd.setBrightness(64);
  xTaskCreatePinnedToCore(taskLcd, "taskLcd", 4096, NULL, 1, NULL, ARDUINO_RUNNING_CORE);
  Serial.printf("LCD initialized.\n");
}


/** Bluetoothの初期設定 */
void beginBle() {
  BLEDevice::init(D_BLE_DEVICE_NAME);
  auto server = BLEDevice::createServer();
  server->setCallbacks(new CBleServerCallbacks());
  auto service = server->createService(D_BLE_SERVICE_UUID);
  auto characteristic = service->createCharacteristic(D_BLE_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_NOTIFY);
  characteristic->addDescriptor(new BLE2902());
  service->start();
  gBleCharacteristic = characteristic;
  Serial.printf("BLE initialized.\n");
}


/** 心拍数センサーの初期設定 */
void beginHrs() {
  attachInterrupt(digitalPinToInterrupt(D_HRS_INPUT_PIN), interruptHrs, RISING);
  Serial.printf("HRS initialized.\n");
}


/** セットアップ */
void setup() {
  SPIFFS.begin();
  M5.begin();
  beginLcd();
  beginBle();
  beginHrs();
}


/** Bluetoothの状態更新 */
void updateBle() {
  if (!gBleIsAdvertising) {
    delay(500);
    BLEDevice::startAdvertising();
    gBleIsAdvertising = true;
    Serial.printf("BLE advertising started.\n");
  }
}


/** ループ */
void loop() {
  M5.update();
  updateBle();
  delay(100);
}
