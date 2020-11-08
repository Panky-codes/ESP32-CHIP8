CHIP8 in ESP32
====================
This is a port of my [CHIP8](https://github.com/Panky-codes/CHIP8) implementation in C++17 for ESP32.

## First look
I tested my implementation of OPCODES with the test ROM from [here](https://github.com/corax89/chip8-test-rom).
<p align="center">
  <img src="doc/first_look.jpg" width="30%" height="30%"/>  
</p>

## TODO

- [x] Finish up keyboard implementation
- [x] Investigate the perf impact for delay more than 10ms and Freertos watch dog timer trigger
- [x] Make a startup screen
- [ ] Update README.md
