#include <algorithm>
#include <bitset>
#include <cstdlib>
#include <sstream>
#include <string>
#include <utility>
#include <map>

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "tft.h"
#include "tftspi.h"
}
#include "display.hpp"

// static defines
static constexpr const char *FILE_TAG = "DISP";
// TODO: Change all the static to be a part of class var
static constexpr const spi_lobo_host_device_t SPI_BUS = TFT_HSPI_HOST;
static constexpr int scale = 4;
static std::bitset<display_size> disp_cache{0};

static const std::map<std::string_view, std::string_view> rom2name{
    {"test_opcode.ch8" , "Test ROM"},
    {"pong.ch8" , "Pong"},
    {"invaders.ch8" , "Space Invaders"},
    {"tetris.ch8" , "Tetris"}};
/*
This function changes the reference axis based on the display midpoint in
ILI9341 display which has a width of 240 pixels and height of 320 pixels The
formula is as follows: x in new ref axis = (TFT_WIDTH/2) + scale/2 * 32 - 4*y y
in new ref axis = (TFT_HEIGHT/2) - scale/2 * 32 + 4*x
*/
static std::pair<int, int> transpose_xy(const int x, const int y) {
  const auto transposed_x =
      (CONFIG_TFT_DISPLAY_WIDTH / 2) + ((scale / 2) * 32) - (4 * y);
  const auto transposed_y =
      (CONFIG_TFT_DISPLAY_HEIGHT / 2) - ((scale / 2) * 64) + (4 * x);

  return {transposed_x, transposed_y};
}

[[nodiscard]] esp_err_t TFTDisp::init() {
  esp_err_t ret;

  TFT_PinsInit();

  // ====  CONFIGURE SPI DEVICES(s)
  // ====================================================================================

  spi_lobo_device_handle_t spi;

  spi_lobo_bus_config_t buscfg = {
      .mosi_io_num = PIN_NUM_MOSI, // set SPI MOSI pin
      .miso_io_num = PIN_NUM_MISO, // set SPI MISO pin
      .sclk_io_num = PIN_NUM_CLK,  // set SPI CLK pin
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = 6 * 1024,
  };
  spi_lobo_device_interface_config_t devcfg = {
      .mode = 0,                         // SPI mode 0
      .clock_speed_hz = 8000000,         // Initial clock out at 8 MHz
      .spics_io_num = -1,                // we will use external CS pin
      .spics_ext_io_num = PIN_NUM_CS,    // external CS pin
      .flags = LB_SPI_DEVICE_HALFDUPLEX, // ALWAYS SET  to HALF DUPLEX MODE!!
                                         // for display spi
  };

  // ==================================================================
  // ==== Initialize the SPI bus and attach the LCD to the SPI bus ====

  ret = spi_lobo_bus_add_device(SPI_BUS, &buscfg, &devcfg, &spi);
  if (ret) {
    ESP_LOGE(FILE_TAG,
             "SPI: Error in adding display device to spi bus (%d)\r\n",
             SPI_BUS);
  }
  tft_disp_spi = spi;

  // ==== Test select/deselect ====
  ret = spi_lobo_device_select(spi, 1);
  assert(ret == ESP_OK);
  ret = spi_lobo_device_deselect(spi);
  assert(ret == ESP_OK);

  // ================================
  // ==== Initialize the Display ====

  TFT_display_init();

  // ==== Set SPI clock used for display operations ====
  spi_lobo_set_speed(spi, DEFAULT_SPI_CLOCK);

  tft_font_rotate = 0;
  tft_text_wrap = 0;
  tft_font_transparent = 0;
  tft_font_forceFixed = 0;
  tft_gray_scale = 0;
  TFT_setGammaCurve(DEFAULT_GAMMA_CURVE);
  TFT_setRotation(PORTRAIT);
  TFT_setFont(DEFAULT_FONT, NULL);
  TFT_resetclipwin();
  TFT_fillScreen(TFT_BLACK);

  return ret;
}

void TFTDisp::setLandscape() { TFT_setRotation(LANDSCAPE); }
void TFTDisp::setPortrait() { TFT_setRotation(PORTRAIT); }

void TFTDisp::clearScreen() { TFT_fillScreen(TFT_BLACK); }
void TFTDisp::drawCheck() {
  TFT_setRotation(LANDSCAPE);
  int y = 4;
  int f = COMIC24_FONT;
  TFT_setFont(f, NULL);
  TFT_print("Welcome to ESP32 CHIP8", CENTER, y);
  y += TFT_getfontheight() + 4;
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  TFTDisp::clearScreen();
  TFT_setRotation(PORTRAIT);
}

void TFTDisp::drawGfx(const std::array<uint8_t, display_size> &gfx) {
  for (uint y = 0; y < display_y; ++y) {
    for (uint x = 0; x < display_x; ++x) {
      const auto [new_x, new_y] = transpose_xy(x, y);
      const auto index_distance = x + (display_x * y);
      const color_t color = (gfx.at(index_distance) == 1) ? TFT_GREEN : tft_bg;
      if ((gfx.at(index_distance) ^ disp_cache[index_distance])) {
        disp_cache.set(index_distance, bool(gfx.at(index_distance)));
        TFT_fillRect(new_x, new_y, 4, 4, color);
        TFT_drawRect(new_x, new_y, 4, 4, color);
      }
    }
  }
}

// TODO: Make the rom list flexible by reading the file names from SPIFFS
void TFTDisp::displayOptions(const std::vector<std::string_view> &rom_list) {
  int y = 4;
  int f = COMIC24_FONT;
  TFT_setFont(f, NULL);
  TFT_print("Welcome to ESP32 CHIP8", CENTER, y);
  y += TFT_getfontheight() + 4;
  f = UBUNTU16_FONT;
  TFT_print("Select an option", 4, y);
  y += TFT_getfontheight() + 4;
  f = DEJAVU18_FONT;
  std::for_each(rom_list.begin(), rom_list.end(),
                [i = 1, y](const auto &rom) mutable {
                  std::ostringstream rom_w_index;
                  rom_w_index << i << ". " << rom2name.at(rom.substr(1));
                  TFT_print(rom_w_index.str().c_str(), 4, y);
                  y += TFT_getfontheight() + 4;
                  ++i;
                });
  vTaskDelay(5 / portTICK_PERIOD_MS);
}
