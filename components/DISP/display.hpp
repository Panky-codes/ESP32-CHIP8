#ifndef DISPLAY_HPP_
#define DISPLAY_HPP_

#include <array>
#include <vector>
#include <string_view>
extern "C" {
#include "esp_log.h"
#include "esp_system.h"
}

static constexpr int display_x = 64;
static constexpr int display_y = 32;
static constexpr int display_size = display_x * display_y;

namespace TFTDisp {
[[nodiscard]] esp_err_t init();
void drawCheck();
void displayOptions(const std::vector<std::string_view> &rom_list);
void clearScreen();
void drawGfx(const std::array<uint8_t, display_size> &gfx);
void setLandscape();
void setPortrait();
} // namespace TFTDisp
#endif // !DISPLAY_HPP_
