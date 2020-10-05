#ifndef CHIP_8_H_
#define CHIP_8_H_

#include <array>
#include <cstdint>
#include <memory>
#include <stack>
#include <string>
#include <vector>

#include "keyboard.hpp"

static constexpr auto display_x = 64;
static constexpr auto display_y = 32;
static constexpr auto display_size = display_x * display_y;
static constexpr bool debug = true;

class chip8 {
public:
  chip8();
  explicit chip8(std::unique_ptr<keyboard> keyPtr);
  void load_memory(const std::vector<uint8_t> &rom_opcodes);
  void load_memory(const std::string &file_name);
  void reset();
  void step_one_cycle();
  [[nodiscard]] std::array<uint8_t, 16> get_V_registers() const;
  [[nodiscard]] std::array<bool, 16> get_Keys_array() const;
  [[nodiscard]] std::array<uint8_t, 4096> get_memory_dump() const;
  [[nodiscard]] std::array<uint8_t, display_size> get_display_pixels() const;
  [[nodiscard]] uint16_t get_prog_counter() const;
  [[nodiscard]] uint8_t get_delay_counter() const;
  [[nodiscard]] uint8_t get_sound_counter() const;
  [[nodiscard]] uint16_t get_I_register() const;
  [[nodiscard]] std::string get_instruction() const;
  [[nodiscard]] std::stack<uint16_t> get_stack() const;
  [[nodiscard]] bool get_display_flag() const;

private:
  std::array<uint8_t, 4096> memory{0};
  std::array<uint8_t, 16> V{0};
  std::stack<uint16_t> hw_stack;
  std::array<uint8_t, display_size> display{0};
  std::array<bool, 16> Keys{false};
  std::unique_ptr<keyboard> numpad{new keyboard()};
  uint16_t I{0};
  const uint16_t prog_mem_begin = 512;
  uint16_t prog_counter{prog_mem_begin};
  std::string instruction{""};
  uint8_t delay_timer{0};
  uint8_t sound_timer{0};
  bool isKeyBPressed{false};
  bool isDisplaySet{false};
};

#endif
