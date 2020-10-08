extern "C" {
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
}
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <random>

#include "ble_server.hpp"
#include "chip8.hpp"
#include "cpu.hpp"
#include "display.hpp"
#include "keyboard.hpp"

// static defines
static constexpr const char *FILE_TAG = "CHIP8";

// Setup BT, disp
[[nodiscard]] static esp_err_t ble_setup() {

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
  return ret;
}

// [[nodiscard]] static esp_err_t select_option() {}

//Input could be the game
static void start(void *params) {
  std::vector<uint8_t> rom{0x61, 0x32};
  chip8 emulator;
  emulator.load_memory(rom);
  auto initial_pc = emulator.get_prog_counter();
  emulator.step_one_cycle();
  // auto actual_V = emulator.get_V_registers();
  // auto final_pc = emulator.get_prog_counter();
  printf("%d", initial_pc);
  while (1) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    ESP_LOGI(FILE_TAG, "I am alive");
  }
}

[[nodiscard]] esp_err_t CHIP8::run() {
  esp_err_t ret = ESP_OK;
  ret = TFTDisp::init();
  if (ret) {
    ESP_LOGE(FILE_TAG, "TFT display init failed %s\n", esp_err_to_name(ret));
  }
  ret = ble_setup();
  if (ret) {
    ESP_LOGE(FILE_TAG, "%s BLE Setup failed", __func__);
  }
  TFTDisp::drawCheck();
  xTaskCreatePinnedToCore(start, "CHIP8", 20000, NULL, 1, NULL, 1);
  return ret;
}