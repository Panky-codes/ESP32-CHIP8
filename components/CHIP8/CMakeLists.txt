idf_component_register(SRCS "chip8.cpp"
                 INCLUDE_DIRS "."
                  REQUIRES BLE VM DISP spiffs)

option(FLASH_SPIFFS "If set, then the rom data will also be flashed" OFF)
if(FLASH_SPIFFS)
    message("Flashing rom along with the app")
    spiffs_create_partition_image(storage ../../externals/rom FLASH_IN_PROJECT)
else()
    spiffs_create_partition_image(storage ../../externals/rom)
endif()

