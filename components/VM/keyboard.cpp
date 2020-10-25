extern "C" {
#include "esp_log.h"
}
#include "keyboard.hpp"
#include <algorithm>

keyboard::keyboard(xQueueHandle numpad_ble) : m_numpad_ble{numpad_ble} {}
// TODO: need to implement this function
void keyboard::storeKeyPress() {
  uint8_t value = 0;
  if (xQueueReceive(m_numpad_ble, &value, (TickType_t)1)) {
    ESP_LOGI("Keypad", "Key pressed : %#2x", value);
    // Ignore other values
    if (value <= 0xF) {
      Keys[value] = true;
    }
  }
}
// The caller should reset the events for Keys and isKeyBPressed
bool keyboard::isKeyVxPressed(const uint8_t &num) {
  // Not the most efficient. Map would be a better data structure
  // but this is done to improve testability with mocks
  storeKeyPress();
  return Keys[num];
}

std::pair<bool, uint8_t> keyboard::whichKeyIndexIfPressed() {
  auto *iter = std::find(Keys.begin(), Keys.end(), true);
  const auto dist = static_cast<uint8_t>(std::distance(Keys.begin(), iter));
  if (dist != Keys.size()) {
    return std::pair<bool, uint8_t>{true, dist};
  }
  return std::pair<bool, uint8_t>{false, dist};
}

void keyboard::clearKeyInput() {
  // std::for_each(Keys.begin(), Keys.end(), [](auto &key) { key = false; });
  std::fill_n(Keys.begin(), Keys.size(), false);
}
