#ifndef KEYBOARD_H_
#define KEYBOARD_H_

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
}
#include <array>
#include <memory>
#include <optional>
#include <cstdint>
#include "observer.hpp"

class keyboard {
public:
  keyboard() = default;
  explicit keyboard(xQueueHandle numpad_ble);
  bool isKeyVxPressed(const uint8_t &num);
  std::optional<uint8_t> whichKeyIndexIfPressed();
  void clearKeyInput();
  void addExitButtonObserver(IObserver* exit_button);
  void storeKeyPress();

private:
  xQueueHandle m_numpad_ble;
  std::array<bool, 16> Keys{false};
  IObserver* m_exitButton;
};

#endif // KEYBOARD_H_
