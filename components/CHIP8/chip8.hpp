#ifndef CHIP_8_H_
#define CHIP_8_H_

#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "esp_err.h"
#include "keyboard.hpp"

namespace CHIP8 {

[[nodiscard]] esp_err_t run();

} // namespace CHIP8

#endif
