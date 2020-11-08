extern "C" {
#include "esp_log.h"
}
#include "keyboard.hpp"
#include <algorithm>

keyboard::keyboard(xQueueHandle numpad_ble) : m_numpad_ble{numpad_ble} {}

void keyboard::storeKeyPress() {
  uint8_t value = 0;
  if (xQueueReceive(m_numpad_ble, &value, (TickType_t)1)) {
    // Ignore other values
    if (value <= 0xF) {
      ESP_LOGD("Keypad", "Key pressed : %#2x", value);
      Keys[value] = true;
    }
    // Exit key is pressed
    if (value == 0xFF) {
      ESP_LOGD("Keypad", "Exit button pressed");
      m_exitButton->update();
    }
  }
}
// The caller should reset the events for Keys and isKeyBPressed
bool keyboard::isKeyVxPressed(const uint8_t &num) {
  // Not the most efficient. Map would be a better data structure
  // but this is done to improve testability with mocks
  storeKeyPress();
  if (Keys[num]) {
    // reset the keys
    Keys[num] = false;
    return true;
  }
  return Keys[num];
}

std::optional<uint8_t> keyboard::whichKeyIndexIfPressed() {
  storeKeyPress();
  auto *iter = std::find(Keys.begin(), Keys.end(), true);
  const auto dist = static_cast<uint8_t>(std::distance(Keys.begin(), iter));
  if (dist != Keys.size()) {
    // reset the Keys
    Keys[dist] = false;
    return dist;
  }
  return std::nullopt;
}

void keyboard::clearKeyInput() {
  std::fill_n(Keys.begin(), Keys.size(), false);
}

void keyboard::addExitButtonObserver(IObserver *exit_button) {
  if (exit_button) {
    m_exitButton = exit_button;
  } else {
    ESP_LOGI("Keypad", "NULL ptr passed as an observer");
  }
}