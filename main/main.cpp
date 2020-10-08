// System headers
#include <cstdio>
#include <cstdlib>
extern "C" {
#include "esp_log.h"
}
// Other components
#include "chip8.hpp"

// static defines
static constexpr const char *FILE_TAG = "MAIN";

extern "C" void app_main(void) {
  esp_err_t ret = CHIP8::run();
  if (ret) {
    ESP_LOGE(FILE_TAG,
             "The initial setup failed from unrecoverable error! Aborting!!");
    abort();
  }
}
