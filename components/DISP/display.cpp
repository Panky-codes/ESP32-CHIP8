#include <cstdlib>
#include <string>

extern "C" {
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tft.h"
#include "tftspi.h"
}
#include "display.hpp"

// static defines
static constexpr const char *FILE_TAG = "DISP";
static constexpr const spi_lobo_host_device_t SPI_BUS = TFT_HSPI_HOST;


[[nodiscard]] esp_err_t TFTDisp::init() {
  esp_err_t ret;

  // === SET GLOBAL VARIABLES ==========================

  // ===================================================
  // ==== Set maximum spi clock for display read    ====
  //      operations, function 'find_rd_speed()'    ====
  //      can be used after display initialization  ====
  tft_max_rdclock = 8000000;
  // ===================================================

  // ====================================================================
  // === Pins MUST be initialized before SPI interface initialization ===
  // ====================================================================
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
  // ---- Detect maximum read speed ----
  tft_max_rdclock = find_rd_speed();

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

void TFTDisp::drawCheck() {
  TFT_setFont(DEFAULT_FONT, NULL);
  TFT_print("CHIP8 DISPLAY CHECK", CENTER, CENTER);
}
