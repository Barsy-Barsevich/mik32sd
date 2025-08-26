#pragma once

#include "mik32sd.h"

int mik32fat_wheels_single_read(void *__restrict cookie, uint32_t sector_addr, uint8_t *dst);
int mik32fat_wheels_single_write(void *__restrict cookie, uint32_t sector_addr, const uint8_t *src);
int mik32fat_wheels_single_erase(void *__restrict cookie, uint32_t sector_addr);