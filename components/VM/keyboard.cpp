#include "keyboard.hpp"
#include <algorithm>

// TODO: need to implement this function
void keyboard::storeKeyPress() {
  // using namespace sf;
  // if (Keyboard::isKeyPressed(Keyboard::Num5)) {
  //   Keys[0x01] = true;
  // } else if (Keyboard::isKeyPressed(Keyboard::Num6)) {
  //   Keys[0x02] = true;
  // } else if (Keyboard::isKeyPressed(Keyboard::Num7)) {
  //   Keys[0x03] = true;
  // } else if (Keyboard::isKeyPressed(Keyboard::Num8)) {
  //   Keys[0x0C] = true;
  // } else if (Keyboard::isKeyPressed(Keyboard::T)) {
  //   Keys[0x04] = true;
  // } else if (Keyboard::isKeyPressed(Keyboard::Y)) {
  //   Keys[0x05] = true;
  // } else if (Keyboard::isKeyPressed(Keyboard::U)) {
  //   Keys[0x06] = true;
  // } else if (Keyboard::isKeyPressed(Keyboard::I)) {
  //   Keys[0x0D] = true;
  // } else if (Keyboard::isKeyPressed(Keyboard::G)) {
  //   Keys[0x07] = true;
  // } else if (Keyboard::isKeyPressed(Keyboard::H)) {
  //   Keys[0x08] = true;
  // } else if (Keyboard::isKeyPressed(Keyboard::J)) {
  //   Keys[0x09] = true;
  // } else if (Keyboard::isKeyPressed(Keyboard::K)) {
  //   Keys[0x0E] = true;
  // } else if (Keyboard::isKeyPressed(Keyboard::B)) {
  //   Keys[0x0A] = true;
  // } else if (Keyboard::isKeyPressed(Keyboard::N)) {
  //   Keys[0x00] = true;
  // } else if (Keyboard::isKeyPressed(Keyboard::M)) {
  //   Keys[0x0B] = true;
  // } else if (Keyboard::isKeyPressed(Keyboard::Comma)) {
  //   Keys[0x0F] = true;
  // } else {
  //   // fmt::print("Unrecognized Key is pressed");
  // }
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
  const auto index = [&]() {
    const auto dist = static_cast<uint8_t>(std::distance(Keys.begin(), iter));
    if (dist != Keys.size())
    {
      return std::pair<bool, uint8_t>{true, dist};
    }
    return std::pair<bool, uint8_t>{false,dist};
  }();
  return index;
}

void keyboard::clearKeyInput() {
  std::for_each(Keys.begin(), Keys.end(), [](auto &key) { key = false; });
}
