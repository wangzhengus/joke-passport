// main/app_jokes.h —— 内置冷笑话库。
#pragma once

#include <stddef.h>

typedef struct {
    const char *text;
} app_joke_t;

size_t app_jokes_count(void);
const app_joke_t *app_jokes_get(size_t index);
