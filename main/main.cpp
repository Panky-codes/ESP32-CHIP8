extern "C" {
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include <stdio.h>
}
#include "ble_server.hpp"
#include "display.hpp"

// static defines
static constexpr const char *FILE_TAG = "MAIN";

extern "C" void app_main(void) {
  esp_err_t ret = ESP_OK;
  // I know this is a memory leak, but the lifetime of
  // the BLE server should last for the complete lifetime of BLE(bluedroid)
  BLEServer *ble = new BLEServer();
  BLEService *service_p = new BLEService{"ESP"};
  ret = ble->init();
  if (ret) {
    ESP_LOGE(FILE_TAG, "%s BLE INIT failed: %s\n", __func__,
             esp_err_to_name(ret));
  }
  ret = ble->addService(service_p);
  if (ret) {
    ESP_LOGE(FILE_TAG, "%s Service setup failed: %s\n", __func__,
             esp_err_to_name(ret));
  }
  ret = ble->startService();
  if (ret) {
    ESP_LOGE(FILE_TAG, "%s Service startup failed: %s\n", __func__,
             esp_err_to_name(ret));
  }
  ret = TFTDisp::init();
  if (ret) {
    ESP_LOGE(FILE_TAG, "TFT display init failed %s\n", esp_err_to_name(ret));
  }
  TFTDisp::drawCheck();
}
