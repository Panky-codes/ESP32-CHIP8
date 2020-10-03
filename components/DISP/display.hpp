#ifndef DISPLAY_HPP_
#define DISPLAY_HPP_

#include <array>
extern "C" {
#include "esp_system.h"
#include "esp_log.h"
}

static constexpr int display_x = 64;
static constexpr int display_y = 32;
static constexpr int display_size = display_x * display_y;

namespace TFTDisp {
[[nodiscard]] esp_err_t init();
void drawCheck();
esp_err_t drawGfx(const std::array<uint8_t, display_size> &gfx);
} // namespace TFTDisp
#endif // !DISPLAY_HPP_
