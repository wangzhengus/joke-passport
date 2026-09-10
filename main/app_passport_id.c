// main/app_passport_id.c —— 编号纯逻辑。
#include "app_passport_id.h"

#include <stdio.h>

int app_passport_id_format(char *out, size_t out_sz,
                           int year, int mon, int day, int hour, int min,
                           uint16_t mac_tail)
{
    if (!out || out_sz < 18) {
        return -1;
    }
    if (year < 2000 || year > 9999) {
        return -1;
    }
    if (mon < 1 || mon > 12 || day < 1 || day > 31) {
        return -1;
    }
    if (hour < 0 || hour > 23 || min < 0 || min > 59) {
        return -1;
    }

    int n = snprintf(out, out_sz, "J%04d%02d%02d%02d%02d%04X",
                     year, mon, day, hour, min, (unsigned)mac_tail);
    if (n < 0 || (size_t)n >= out_sz) {
        return -1;
    }
    return n;
}
