#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include <array>
#include <cstdint>

//Think of a better API
class keyboard {
public:
  virtual bool isKeyVxPressed(const uint8_t &num);
  virtual std::pair<bool,uint8_t> whichKeyIndexIfPressed();
  virtual void clearKeyInput();
  virtual ~keyboard() = default;

private:
  std::array<bool, 16> Keys{false};
  void storeKeyPress();
};

#endif // KEYBOARD_H_
