#ifndef __UTIL_TEXT_H__
#define __UTIL_TEXT_H__

#include <stdint.h>
#include <stdbool.h>

uint32_t utf8_decode(char **str);

bool str_startswith(const char* str, const char* prefix);

#endif