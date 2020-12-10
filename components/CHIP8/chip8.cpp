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
#include <string>
#include <string_view>

#include "ble_server.hpp"
#include "chip8.hpp"
#include "cpu.hpp"
#include "display.hpp"
#include "keyboard.hpp"

// static defines
static constexpr const char *FILE_TAG = "CHIP8";
static constexpr int NR_OF_ROMS = 2;
static constexpr std::array<std::string_view, NR_OF_ROMS> TEST_ROM = {
    "/test_opcode.ch8", "/pong.ch8"};
enum class EMU_STATE { SELECT_OPTION, PLAY_GAME };

// Setup BT, disp
[[nodiscard]] static esp_err_t ble_setup(xQueueHandle &numpad) {
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
  numpad = service_p->getQueueHandle();
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

static void get_option_selection(keyboard *numpad_handle, int &rom_selection) {
  TFTDisp::setLandscape();
  TFTDisp::displayOptions(TEST_ROM);
  bool option_selected = false;
  // Wait forever until a selection is made
  while (!option_selected) {
    numpad_handle->storeKeyPress();
    const auto opt = numpad_handle->whichKeyIndexIfPressed();
    if (opt && ((opt.value() <= NR_OF_ROMS) && (opt.value() != 0))) {
      rom_selection = opt.value() - 1;
      option_selected = true;
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  // flush the key input
  numpad_handle->clearKeyInput();
  TFTDisp::setPortrait();
}

static void start(void *params) {
  xQueueHandle numpad_queue = params;
  int rom_selection;
  EMU_STATE state = EMU_STATE::SELECT_OPTION;

  std::unique_ptr<ExitButton> exit_button = std::make_unique<ExitButton>();
  std::unique_ptr<keyboard> numpad = std::make_unique<keyboard>(numpad_queue);
  numpad->addExitButtonObserver(exit_button.get());
  chip8 emulator{numpad.get()};

  while (1) {
    switch (state) {
    case EMU_STATE::SELECT_OPTION: {
      TFTDisp::clearScreen();
      get_option_selection(numpad.get(), rom_selection);
      std::string rom_file = CONFIG_SPIFFS_BASE_DIR;
      rom_file += TEST_ROM[rom_selection];
      emulator.load_memory(rom_file);
      TFTDisp::clearScreen();
      state = EMU_STATE::PLAY_GAME;
      break;
    }
    case EMU_STATE::PLAY_GAME: {
      while (!exit_button->isPressed()) {
        emulator.step_one_cycle();
        // To avoid drawing in every cycle
        if (emulator.get_display_flag()) {
          TFTDisp::drawGfx(emulator.get_display_pixels());
        }
        vTaskDelay(2 / portTICK_PERIOD_MS);
        numpad->storeKeyPress();
      }
      // flush the key input
      numpad->clearKeyInput();
      state = EMU_STATE::SELECT_OPTION;
      break;
    }
    default:
      break;
    }
  }
}

[[nodiscard]] esp_err_t CHIP8::run() {
  esp_err_t ret = ESP_OK;
  xQueueHandle numpad;
  ret = TFTDisp::init();
  if (ret) {
    ESP_LOGE(FILE_TAG, "TFT display init failed %s\n", esp_err_to_name(ret));
  }
  ret = ble_setup(numpad);
  if (ret) {
    ESP_LOGE(FILE_TAG, "%s BLE Setup failed", __func__);
  }
  ret = setup_fs();
  xTaskCreatePinnedToCore(start, "CHIP8", 20000, numpad,
                          configMAX_PRIORITIES - 1, NULL, 1);
  return ret;
}
