extern "C" {
#include "esp_log.h"
#include "esp_spiffs.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
}
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <random>
#include <string>

#include "ble_server.hpp"
#include "chip8.hpp"
#include "cpu.hpp"
#include "display.hpp"
#include "keyboard.hpp"

// static defines
static constexpr const char *FILE_TAG = "CHIP8";
static constexpr const char *TEST_ROM[] = {"/test_opcode.ch8", "/pong.ch8"};

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
[[nodiscard]] static esp_err_t setup_fs() {
  ESP_LOGI(FILE_TAG, "Initializing SPIFFS");

  esp_vfs_spiffs_conf_t conf = {.base_path = CONFIG_SPIFFS_BASE_DIR,
                                .partition_label = NULL,
                                .max_files = 5,
                                .format_if_mount_failed = false};
  esp_err_t ret = esp_vfs_spiffs_register(&conf);

  if (ret != ESP_OK) {
    if (ret == ESP_FAIL) {
      ESP_LOGE(FILE_TAG, "Failed to mount or format filesystem");
    } else if (ret == ESP_ERR_NOT_FOUND) {
      ESP_LOGE(FILE_TAG, "Failed to find SPIFFS partition");
    } else {
      ESP_LOGE(FILE_TAG, "Failed to initialize SPIFFS (%s)",
               esp_err_to_name(ret));
    }
    return ret;
  }
  return ret;
}

// [[nodiscard]] static esp_err_t select_option() {}

// Input could be the game, Queuehandle
static void start(void *params) {
  chip8 emulator;
  std::string rom_file = CONFIG_SPIFFS_BASE_DIR;
  rom_file += TEST_ROM[0];
  emulator.load_memory(rom_file);
  TFTDisp::clearScreen();
  while (1) {
    emulator.step_one_cycle();
    //To avoid drawing in every cycle
    if (emulator.get_display_flag()) {
      TFTDisp::drawGfx(emulator.get_display_pixels());
    }
    // Can do a yield(Faster) or task delay to avoid watchdog timeout
    taskYIELD();
    // vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

// TODO: If the initial conditions, return to main
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
  ret = setup_fs();
  TFTDisp::drawCheck();
  xTaskCreatePinnedToCore(start, "CHIP8", 20000, NULL, 5, NULL, 1);
  return ret;
}