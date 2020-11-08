#ifndef CPU_HPP_
#define CPU_HPP_

#include <array>
#include <cstdint>
#include <memory>
#include <stack>
#include <string>
#include <string_view>
#include <vector>

#include "keyboard.hpp"
#include "display.hpp"

class chip8 {
public:
  chip8();
  explicit chip8(keyboard* keyPtr);
  void load_memory(const std::vector<uint8_t> &rom_opcodes);
  void load_memory(std::string_view file_name);
  void reset();
  void step_one_cycle();
  [[nodiscard]] std::array<uint8_t, 16> get_V_registers() const;
  [[nodiscard]] std::array<bool, 16> get_Keys_array() const;
  [[nodiscard]] std::array<uint8_t, 4096> get_memory_dump() const;
  [[nodiscard]] const std::array<uint8_t, display_size>& get_display_pixels() const;
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
  keyboard* numpad;
  uint16_t I{0};
  const uint16_t prog_mem_begin = 512;
  uint16_t prog_counter{prog_mem_begin};
  std::string instruction{""};
  uint8_t delay_timer{0};
  uint8_t sound_timer{0};
  bool isKeyBPressed{false};
  bool isDisplaySet{false};
  void reset_internal_states();
};

#endif // CPU_HPP
