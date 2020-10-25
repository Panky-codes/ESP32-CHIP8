#ifndef KEYBOARD_H_
#define KEYBOARD_H_

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
}
#include <array>
#include <cstdint>

// Think of a better API
class keyboard {
public:
  keyboard() = default;
  explicit keyboard(xQueueHandle numpad_ble);
  virtual bool isKeyVxPressed(const uint8_t &num);
  virtual std::pair<bool, uint8_t> whichKeyIndexIfPressed();
  virtual void clearKeyInput();
  virtual ~keyboard() = default;

private:
  xQueueHandle m_numpad_ble;
  std::array<bool, 16> Keys{false};
  void storeKeyPress();
};

#endif // KEYBOARD_H_
