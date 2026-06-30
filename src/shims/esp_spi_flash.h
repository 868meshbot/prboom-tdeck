// Pass through to the real SDK header — Arduino ESP32 still ships esp_spi_flash.h
// with the old IDF4 API (spi_flash_mmap_handle_t, SPI_FLASH_MMAP_DATA, etc.).
// This file exists only so -I src/shims doesn't hide the real header from
// esp_partition.h's internal #include "esp_spi_flash.h".
#pragma once
#include_next "esp_spi_flash.h"
